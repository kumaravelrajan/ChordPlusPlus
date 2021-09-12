#include "Peer.h"
#include <stack>
#include <util.h>
#include <centralLogControl.h>

using dht::PeerImpl;
using dht::Peer;
using dht::Node;


PeerImpl::PeerImpl(std::shared_ptr<NodeInformation> nodeInformation, GetSuccessorMethod getSuccessorMethod) :
    m_getSuccessorMethod{getSuccessorMethod},
    m_nodeInformation{std::move(nodeInformation)} {}

// Server methods

::kj::Promise<void> PeerImpl::getSuccessor(GetSuccessorContext context)
{
    SPDLOG_TRACE("received getSuccessor request");
    auto id = idFromReader(context.getParams().getId());
    return getSuccessor(id).then([KJ_CPCAP(context)](const std::optional<NodeInformation::Node> &successor) mutable {
        if (!successor) {
            context.getResults().getNode().setEmpty();
        } else {
            auto node = context.getResults().getNode().getValue();
            node.setIp(successor->getIp());
            node.setPort(successor->getPort());
            auto id = successor->getId();
            node.setId(
                capnp::Data::Builder(kj::heapArray<kj::byte>(id.begin(), id.end())));
        }
    });
}

::kj::Promise<void> PeerImpl::getClosestPreceding(GetClosestPrecedingContext context)
{
    auto id = idFromReader(context.getParams().getId());
    buildNode(context.getResults().getPreceding(), getClosestPreceding(id));
    buildNode(context.getResults().getDirectSuccessor(), m_nodeInformation->getSuccessor());
    return kj::READY_NOW;
}

::kj::Promise<void> PeerImpl::getPredecessor(GetPredecessorContext context)
{
    SPDLOG_TRACE("received getPredecessor request");
    auto pred = m_nodeInformation->getPredecessor();
    if (pred) {
        auto node = context.getResults().getNode().getValue();
        node.setIp(pred->getIp());
        node.setPort(pred->getPort());
        node.setId(containerToArray<kj::byte>(pred->getId()));
    } else {
        context.getResults().getNode().setEmpty();
    }
    return kj::READY_NOW;
}

::kj::Promise<void> PeerImpl::notify(NotifyContext context)
{
    SPDLOG_TRACE("received notify request");

    auto node = nodeFromReader(context.getParams().getNode());
    auto pred = m_nodeInformation->getPredecessor();

    if (!pred ||
        util::is_in_range_loop(
            node.getId(),
            pred->getId(),
            m_nodeInformation->getId(),
            false, false
        )) {
        m_nodeInformation->setPredecessor(node);
    }

    return kj::READY_NOW;
}

::kj::Promise<void> PeerImpl::getData(GetDataContext context)
{
    SPDLOG_TRACE("received getData request");

    std::vector<uint8_t> key{context.getParams().getKey().begin(), context.getParams().getKey().end()};
    auto value = m_nodeInformation->getData(key);
    if (value) {
        context.getResults().getData().setValue(
            capnp::Data::Builder(kj::heapArray<kj::byte>(value->begin(), value->end())));
    } else {
        context.getResults().getData().setEmpty();
    }
    return kj::READY_NOW;
}

::kj::Promise<void> PeerImpl::setData(SetDataContext context)
{
    SPDLOG_TRACE("received setData request");

    // TODO: only store if this node is responsible for the key.
    //       But we'll deal with hardening against attacks later.
    auto ttl_ = context.getParams().getTtl();
    std::chrono::system_clock::duration ttl =
        ttl_ > 0 ?
        std::chrono::seconds(ttl_) :
        std::chrono::system_clock::duration::max();
    auto key = context.getParams().getKey();
    auto value = context.getParams().getValue();
    m_nodeInformation->setData(
        {key.begin(), key.end()},
        {value.begin(), value.end()},
        ttl
    );
    return kj::READY_NOW;
}

::kj::Promise<void> dht::PeerImpl::getDataItemsOnJoin(GetDataItemsOnJoinContext context)
{
    /* Check if current node is predecessor of node in GetDataItemsOnJoinParams */
    auto newNode = nodeFromReader(context.getParams().getNewNode());

    if (!util::is_in_range_loop(newNode.getId(), m_nodeInformation->getPredecessor()->getId(),
                                m_nodeInformation->getId(), false, false)) {
        SPDLOG_INFO("New Node must be in between predecessor an this node.");
        return kj::READY_NOW;
    }

    /* Get data items for which predecessor of current node is responsible. */
    auto dataForNewNode = m_nodeInformation->getDataItemsForNodeId(newNode);

    /* Delete data items from current node which have been assigned to predecessor. */
    std::vector<std::vector<uint8_t>> keyOfDataItemsAssignedToPredecessor;
    for (const auto &s: *dataForNewNode) {
        keyOfDataItemsAssignedToPredecessor.push_back(s.first);
    }
    m_nodeInformation->deleteDataAssignedToPredecessor(keyOfDataItemsAssignedToPredecessor);

    if (dataForNewNode) {
        auto iter = dataForNewNode->begin();

        auto s = context.getResults().initListOfDataItems(static_cast<kj::uint>(dataForNewNode->size()));

        // Iterate through map and store values in ::capnp::List
        for (kj::uint i = 0; i < dataForNewNode->size(); ++i) {
            s[i].setKey(kj::heapArray<kj::byte>(iter->first.begin(), iter->first.end()));                 // NOLINT
            s[i].setData(kj::heapArray<kj::byte>(iter->second.first.begin(), iter->second.first.end()));  // NOLINT
            s[i].setExpires(                                                                                   // NOLINT
                static_cast<size_t>(std::chrono::duration_cast<std::chrono::seconds>(
                    iter->second.second.time_since_epoch()).count())
            );
            ++iter;
        }
    }

    return kj::READY_NOW;
}

::kj::Promise<void> PeerImpl::getPoWPuzzleOnJoin(GetPoWPuzzleOnJoinContext context)
{
    auto newNode = nodeFromReader(context.getParams().getNewNode());
    std::string strproofOfWorkPuzzle = newNode.getIp() + ":" + std::to_string(newNode.getPort());
    context.getResults().setProofOfWorkPuzzle(strproofOfWorkPuzzle); // NOLINT
    context.getResults().setDifficulty(m_nodeInformation->getDifficulty()); // NOLINT

    return kj::READY_NOW;
}

::kj::Promise<void> PeerImpl::sendPoWPuzzleResponseToBootstrapAndGetSuccessor(
    SendPoWPuzzleResponseToBootstrapAndGetSuccessorContext context)
{

    std::string sPoW_Reponse(context.getParams().getProofOfWorkPuzzleResponse());

    std::string sPoW_HashOfResponse(context.getParams().getHashOfproofOfWorkPuzzleResponse());

    NodeInformation::Node newNode = nodeFromReader(context.getParams().getNewNode());

    std::string sTestString = newNode.getIp() + ":" + std::to_string(newNode.getPort());

    // If new node is 127.0.0.1:6007, PoW_Reponse contains some string appended to "127.0.0.1:6007". 
    // Check if correct PoW_Reponse has been handed over by new node.
        KJ_ASSERT(sPoW_Reponse.substr(0, sTestString.size()) == sTestString) {
            return kj::READY_NOW;
        }

    std::string sCalculatedHashOfPuzzleResponse = util::bytedump(util::hash_sha1(sPoW_Reponse), SHA_DIGEST_LENGTH);
    sCalculatedHashOfPuzzleResponse.erase(
        std::remove_if(sCalculatedHashOfPuzzleResponse.begin(), sCalculatedHashOfPuzzleResponse.end(), ::isspace),
        sCalculatedHashOfPuzzleResponse.end());

    // Check if hash(sPoW_Reponse) == sPoW_HashOfResponse
        KJ_ASSERT(sCalculatedHashOfPuzzleResponse == sPoW_HashOfResponse) {
            return kj::READY_NOW;
        }

    // Check if sPoW_HashOfResponse has correct number of leading zeroes
    std::string strDifficulty(static_cast<size_t>(m_nodeInformation->getDifficulty()), '0');
        KJ_ASSERT(
        sPoW_HashOfResponse.substr(0, static_cast<size_t>(m_nodeInformation->getDifficulty())) == strDifficulty) {
            return kj::READY_NOW;
        }

    // If all checks passed, get successor of new node
    return getSuccessor(newNode.getId()).then(
        [KJ_CPCAP(context)](std::optional<NodeInformation::Node> &&successor) mutable {
            buildNode(context.getResults().getSuccessorOfNewNode(), successor);
        });
}

// Conversion

NodeInformation::Node PeerImpl::nodeFromReader(Node::Reader value)
{
    return NodeInformation::Node{
        value.getIp(),
        value.getPort()
        //
        //, idFromReader(value.getId())
    };
}

std::optional<NodeInformation::Node> PeerImpl::nodeFromReader(Optional<Node>::Reader node)
{
    if (node.which() == Optional<Node>::EMPTY)
        return {};
    return nodeFromReader(node.getValue());
}

void PeerImpl::buildNode(Node::Builder builder, const NodeInformation::Node &node)
{
    builder.setIp(node.getIp());
    builder.setPort(node.getPort());
    builder.setId(containerToArray<kj::byte>(node.getId()));
}

void PeerImpl::buildNode(Optional<Node>::Builder builder, const std::optional<NodeInformation::Node> &node)
{
    if (node)
        buildNode(builder.getValue(), *node);
    else
        builder.setEmpty();
}

NodeInformation::id_type PeerImpl::idFromReader(capnp::Data::Reader id)
{
    NodeInformation::id_type ret{};
    std::copy_n(id.begin(),
                std::min(id.size(), ret.size()),
                ret.begin());
    return ret;
}

// Interface

::kj::Promise<std::optional<NodeInformation::Node>> PeerImpl::getSuccessor(NodeInformation::id_type id)
{
    LOG_GET

    // If this node is requested
    auto pred = m_nodeInformation->getPredecessor();
    if (pred &&
        util::is_in_range_loop(
            id,
            pred->getId(), m_nodeInformation->getId(),
            false, true
        )) {
        return std::optional<NodeInformation::Node>{m_nodeInformation->getNode()};
    }

    // If next node is requested
    auto successor = m_nodeInformation->getSuccessor();
    if (successor &&
        util::is_in_range_loop(
            id,
            m_nodeInformation->getId(), successor->getId(),
            false, true
        )) {
        // Check if successor is online
        auto client = getClient(successor->getIp(), successor->getPort());
        auto cap = client->getMain<Peer>();
        auto req = cap.getPredecessorRequest();
        return req.send().then(
            [client = kj::mv(client), successor](capnp::Response<Peer::GetPredecessorResults> &&) {
                return successor;
            }, [LOG_CAPTURE](const kj::Exception &e) {
                LOG_DEBUG("Exception in request\n\t\t{}", e.getDescription().cStr());
                return std::optional<NodeInformation::Node>{};
            }
        );
    }

    if (m_getSuccessorMethod == GetSuccessorMethod::PASS_ON) {
        // Otherwise, pass the request to the closest preceding finger
        auto closest_preceding = getClosestPreceding(id);

        if (!closest_preceding) {
            return std::optional<NodeInformation::Node>{};
        }

        auto client = getClient(closest_preceding->getIp(), closest_preceding->getPort());
        auto cap = client->getMain<Peer>();
        auto req = cap.getSuccessorRequest();
        req.setId(capnp::Data::Builder{kj::heapArray<kj::byte>(id.begin(), id.end())});
        return req.send().attach(kj::mv(client)).then([](capnp::Response<Peer::GetSuccessorResults> &&response) {
            return nodeFromReader(response.getNode());
        }, [LOG_CAPTURE](const kj::Exception &e) {
            LOG_DEBUG("Exception in request\n\t\t{}", e.getDescription().cStr());
            return std::optional<NodeInformation::Node>{};
        });
    }

    // Core Algorithm of Chord

    auto hops = std::make_shared<std::stack<NodeInformation::Node>>(std::deque{m_nodeInformation->getNode()});
    auto distrusted = std::make_shared<std::unordered_set<NodeInformation::Node, NodeInformation::Node::Node_hash>>();
    // NOTE: this could be initialized using the rating system.
    //   i.e.: With 50 random nodes from the 1000 worst rated nodes.
    //   At the end, this set can be sent to the rating server.

    auto getSuccessorAlgorithm = [LOG_CAPTURE, this, id, hops, distrusted](
        auto getSuccessorAlgorithm
    ) mutable -> kj::Promise<std::optional<NodeInformation::Node>> {
        if (hops->empty())
            return std::optional<NodeInformation::Node>{};
        auto current = hops->top();
        return getClosestPrecedingHelper(current, id, distrusted).then(
            [LOG_CAPTURE, this, getSuccessorAlgorithm, id, hops, distrusted, current](
                ClosestPrecedingPair &&result) mutable -> kj::Promise<std::optional<NodeInformation::Node>> {
                // In between node has been found:
                if (result.closestPreceding) {
                    hops->push(*result.closestPreceding);
                    return getSuccessorAlgorithm(getSuccessorAlgorithm);
                }
                // Responsible node has been found:
                if (result.successor) return result.successor;
                distrusted->insert(current);
                hops->pop();
                return getSuccessorAlgorithm(getSuccessorAlgorithm);
            }
        );
    };

    return getSuccessorAlgorithm(getSuccessorAlgorithm);
}

std::optional<NodeInformation::Node> PeerImpl::getClosestPreceding(NodeInformation::id_type id)
{
    for (size_t i = NodeInformation::key_bits; i >= 1ull; --i) {
        auto finger = m_nodeInformation->getFinger(i - 1);
        if (finger && util::is_in_range_loop(
            finger->getId(), m_nodeInformation->getId(), id,
            false, false
        ))
            return finger;
    }
    return {};
}

std::optional<std::vector<uint8_t>>
PeerImpl::getData(const NodeInformation::Node &node, const std::vector<uint8_t> &key)
{
    LOG_GET
    if (node == m_nodeInformation->getNode()) {
        LOG_TRACE("Get from this node");
        return m_nodeInformation->getData(key);
    } else {
        auto client = getClient(node.getIp(), node.getPort());
        auto cap = client->getMain<Peer>();
        auto req = cap.getDataRequest();
        req.setKey(capnp::Data::Builder(kj::heapArray<kj::byte>(key.begin(), key.end())));
        return req.send().then([LOG_CAPTURE](capnp::Response<Peer::GetDataResults> &&response) {
            auto data = response.getData();

            if (data.which() == Optional<capnp::Data>::EMPTY) {
                return std::optional<std::vector<uint8_t>>{};
            }
            LOG_TRACE("Got Data");
            return std::optional<std::vector<uint8_t>>{{data.getValue().begin(), data.getValue().end()}};
        }, [LOG_CAPTURE](const kj::Exception &e) {
            LOG_DEBUG("Exception in request\n\t\t{}", e.getDescription().cStr());
            return std::optional<std::vector<uint8_t>>{};
        }).wait(client->getWaitScope());
    }
}

void PeerImpl::setData(
    const NodeInformation::Node &node,
    const std::vector<uint8_t> &key, const std::vector<uint8_t> &value,
    uint16_t ttl)
{
    LOG_GET
    auto ttl_seconds = ttl > 0
                       ? std::chrono::seconds(ttl)
                       : std::chrono::system_clock::duration::max();

    if (node == m_nodeInformation->getNode()) {
        LOG_TRACE("Store in this node");
        m_nodeInformation->setData(key, value, ttl_seconds);
    } else {
        auto client = getClient(node.getIp(), node.getPort());
        auto cap = client->getMain<Peer>();
        auto req = cap.setDataRequest();
        req.setKey(capnp::Data::Builder(kj::heapArray<kj::byte>(key.begin(), key.end())));
        req.setValue(capnp::Data::Builder(kj::heapArray<kj::byte>(value.begin(), value.end())));
        req.setTtl(ttl);
        return req.send().then([LOG_CAPTURE](capnp::Response<Peer::SetDataResults> &&) {
            LOG_TRACE("got response");
        }, [LOG_CAPTURE](const kj::Exception &e) {
            LOG_DEBUG("Exception in request\n\t\t{}", e.getDescription().cStr());
        }).wait(client->getWaitScope());
    }
}

// Helpers

::kj::Promise<PeerImpl::ClosestPrecedingPair>
PeerImpl::getClosestPrecedingHelper(const NodeInformation::Node &node, const NodeInformation::id_type &id,
                                    const std::shared_ptr<std::unordered_set<NodeInformation::Node, NodeInformation::Node::Node_hash>> &distrusted)
{
    LOG_GET;
    auto checkResult = [LOG_CAPTURE, this, node, distrusted, id](
        ClosestPrecedingPair result) mutable -> ::kj::Promise<PeerImpl::ClosestPrecedingPair> {
        // Verify that preceding is between the asked node and the requested id,
        // and if directSuccessor is populated, make sure it is responsible for the id!
        if (result.closestPreceding &&
            !util::is_in_range_loop(result.closestPreceding->getId(), node.getId(), id, false, false)) {
            LOG_INFO("Returned closest preceding is not in between node and id!");
            result.closestPreceding.reset();
        }
        if (result.successor && *result.successor == m_nodeInformation->getNode()) {
            result.successor.reset();
        } else if (result.successor) {
            auto client = getClient(result.successor->getIp(), result.successor->getPort());
            auto cap = client->getMain<Peer>();
            auto req = cap.getPredecessorRequest();
            return req.send().attach(kj::mv(client)).then(
                [LOG_CAPTURE, node, id, result](capnp::Response<Peer::GetPredecessorResults> &&res) mutable {
                    auto pred = nodeFromReader(res.getNode());
                    if (!pred || (pred && !util::is_in_range_loop(id, pred->getId(), node.getId(), false, true))) {
                        LOG_INFO("Returned direct successor is not responsible for the id [{}]!",
                                 util::hexdump(id, 32, false, false));
                        result.successor.reset();
                    }
                    return result;
                }
            );
        }
        return result;
    };

    auto impl = [LOG_CAPTURE, this, node, distrusted, checkResult](
        const NodeInformation::id_type &id) mutable -> ::kj::Promise<PeerImpl::ClosestPrecedingPair> {

        if (node == m_nodeInformation->getNode()) {
            return checkResult(ClosestPrecedingPair{
                getClosestPreceding(id),
                m_nodeInformation->getSuccessor()
            });
        } else {
            auto client = getClient(node.getIp(), node.getPort());
            auto cap = client->getMain<Peer>();
            auto req = cap.getClosestPrecedingRequest();
            req.setId(containerToArray<kj::byte>(id));
            return req.send().attach(kj::mv(client)).then(
                [](capnp::Response<Peer::GetClosestPrecedingResults> &&res) {
                    return ClosestPrecedingPair{
                        nodeFromReader(res.getPreceding()),
                        nodeFromReader(res.getDirectSuccessor())
                    };
                }, [LOG_CAPTURE](kj::Exception &&e) {
                    LOG_DEBUG("Exception in request\n\t\t{}", e.getDescription().cStr());
                    return ClosestPrecedingPair{};
                }
            ).then([checkResult](ClosestPrecedingPair &&result) mutable {
                return checkResult(result);
            });
        }
    };

    auto iterate = [LOG_CAPTURE, this, node, distrusted, impl](
        auto iterate,
        const NodeInformation::id_type &id) mutable -> ::kj::Promise<PeerImpl::ClosestPrecedingPair> {
        return impl(id).then([LOG_CAPTURE, this, node, distrusted, impl, iterate](
            PeerImpl::ClosestPrecedingPair &&result) mutable -> ::kj::Promise<PeerImpl::ClosestPrecedingPair> {
            if (result.closestPreceding && distrusted->contains(*result.closestPreceding)) {
                LOG_DEBUG("Skipping closest preceding because of distrust!");
                return iterate(iterate, result.closestPreceding->getId());
            }
            return result;
        });
    };

    return iterate(iterate, id);
}

void PeerImpl::getDataItemsOnJoinHelper(std::optional<NodeInformation::Node> successorNode)
{
    LOG_GET
    auto client = getClient(successorNode->getIp(), successorNode->getPort());
    auto cap = client->getMain<Peer>();
    auto req = cap.getDataItemsOnJoinRequest();
    buildNode(req.getNewNode(), m_nodeInformation->getNode());

    /* Start RPC */
    req.send().then([LOG_CAPTURE, this](capnp::Response<Peer::GetDataItemsOnJoinResults> &&Response) {
        std::map<std::vector<uint8_t>, std::pair<std::vector<uint8_t>, uint16_t>> mapDataItemsToReturn;

        Response.getListOfDataItems().size();
        for (auto individualDataItem: Response.getListOfDataItems()) {
            const std::vector<uint8_t> key(individualDataItem.getKey().begin(), individualDataItem.getKey().end());
            std::vector<uint8_t> data(individualDataItem.getData().begin(), individualDataItem.getData().end());
            size_t expires_since_epoch = individualDataItem.getExpires();
            std::chrono::system_clock::time_point expires{std::chrono::seconds{expires_since_epoch}};
            m_nodeInformation->setDataExpires(key, data, expires);
        }
        LOG_TRACE("got response from GetDataItemsOnJoin");
    }, [LOG_CAPTURE](const kj::Exception &e) {
        LOG_DEBUG(e.getDescription().cStr());
    }).wait(client->getWaitScope());
}

kj::Own<capnp::EzRpcClient> PeerImpl::getClient(const std::string &ip, uint16_t port)
{
    return kj::heap<capnp::EzRpcClient>(ip, port);
}
