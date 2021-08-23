#include "Peer.h"
#include <capnp/ez-rpc.h>
#include <util.h>
#include <centralLogControl.h>

using dht::PeerImpl;
using dht::Peer;
using dht::Node;


PeerImpl::PeerImpl(std::shared_ptr<NodeInformation> nodeInformation) :
    m_nodeInformation{std::move(nodeInformation)} {}

// Server methods

::kj::Promise<void> PeerImpl::getSuccessor(GetSuccessorContext context)
{
    SPDLOG_TRACE("received getSuccessor request");
    auto id_ = context.getParams().getId();
    NodeInformation::id_type id{};
    std::copy_n(id_.begin(),
                std::min(id_.size(), id.size()),
                id.begin());
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

::kj::Promise<void> PeerImpl::getPredecessor(GetPredecessorContext context)
{
    SPDLOG_TRACE("received getPredecessor request");
    auto pred = m_nodeInformation->getPredecessor();
    if (pred) {
        auto node = context.getResults().getNode().getValue();
        node.setIp(pred->getIp());
        node.setPort(pred->getPort());
        auto id = pred->getId();
        node.setId(capnp::Data::Builder(kj::heapArray<kj::byte>(id.begin(), id.end())));
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
    for (auto s : *dataForNewNode) {
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
    context.getResults().setProofOfWorkPuzzle(strproofOfWorkPuzzle);                                           // NOLINT
    context.getResults().setDifficulty(m_nodeInformation->getDifficulty());                               // NOLINT

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
        KJ_ASSERT(sPoW_Reponse.substr(0, sTestString.size()) == sTestString);
    std::string sCalculatedHashOfPuzzleResponse = util::bytedump(util::hash_sha1(sPoW_Reponse), SHA_DIGEST_LENGTH);
    sCalculatedHashOfPuzzleResponse.erase(
        std::remove_if(sCalculatedHashOfPuzzleResponse.begin(), sCalculatedHashOfPuzzleResponse.end(), ::isspace),
        sCalculatedHashOfPuzzleResponse.end());

    // Check if hash(sPoW_Reponse) == sPoW_HashOfResponse
        KJ_ASSERT(sCalculatedHashOfPuzzleResponse == sPoW_HashOfResponse);

    // Check if sPoW_HashOfResponse has correct number of leading zeroes
    std::string strDifficulty(static_cast<size_t>(m_nodeInformation->getDifficulty()), '0');
        KJ_ASSERT(
        sPoW_HashOfResponse.substr(0, static_cast<size_t>(m_nodeInformation->getDifficulty())) == strDifficulty);

    // If all checks passed, get successor of new node
    return getSuccessor(newNode.getId()).then(
        [KJ_CPCAP(context)](const std::optional<NodeInformation::Node> &successor) mutable {
            if (!successor) {
                context.getResults().getSuccessorOfNewNode().setEmpty();
            } else {
                auto node = context.getResults().getSuccessorOfNewNode().getValue();
                node.setIp(successor->getIp());
                node.setPort(successor->getPort());
                auto id = successor->getId();
                node.setId(
                    capnp::Data::Builder(kj::heapArray<kj::byte>(id.begin(), id.end())));
            }
        });
}

// Conversion

NodeInformation::Node PeerImpl::nodeFromReader(Node::Reader value)
{
    auto res_id_ = value.getId();
    NodeInformation::id_type res_id{};
    std::copy_n(res_id_.begin(),
                std::min(res_id_.size(), res_id.size()),
                res_id.begin());
    return NodeInformation::Node{
        value.getIp(),
        value.getPort(),
        res_id
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
    auto id = node.getId();
    builder.setId(capnp::Data::Builder(kj::heapArray<kj::byte>(id.begin(), id.end())));
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
        auto client = kj::heap<capnp::EzRpcClient>(successor->getIp(), successor->getPort());
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

    // Otherwise, pass the request to the closest preceding finger
    auto closest_preceding = getClosestPreceding(id);

    if (!closest_preceding) {
        return std::optional<NodeInformation::Node>{};
    }

    auto client = kj::heap<capnp::EzRpcClient>(closest_preceding->getIp(), closest_preceding->getPort());
    auto cap = client->getMain<Peer>();
    auto req = cap.getSuccessorRequest();
    req.setId(capnp::Data::Builder{kj::heapArray<kj::byte>(id.begin(), id.end())});
    return req.send().then([client = kj::mv(client)](capnp::Response<Peer::GetSuccessorResults> &&response) {
        return nodeFromReader(response.getNode());
    }, [LOG_CAPTURE](const kj::Exception &e) {
        LOG_DEBUG("Exception in request\n\t\t{}", e.getDescription().cStr());
        return std::optional<NodeInformation::Node>{};
    });
}

std::optional<NodeInformation::Node> PeerImpl::getClosestPreceding(NodeInformation::id_type id)
{
    for (size_t i = NodeInformation::key_bits; i >= 1ull; --i) {
        if (m_nodeInformation->getFinger(i - 1) && util::is_in_range_loop(
            m_nodeInformation->getFinger(i - 1)->getId(), m_nodeInformation->getId(), id,
            false, false
        ))
            return m_nodeInformation->getFinger(i - 1);
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
        capnp::EzRpcClient client{node.getIp(), node.getPort()};
        auto cap = client.getMain<Peer>();
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
        }).wait(client.getWaitScope());
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
        capnp::EzRpcClient client{node.getIp(), node.getPort()};
        auto cap = client.getMain<Peer>();
        auto req = cap.setDataRequest();
        req.setKey(capnp::Data::Builder(kj::heapArray<kj::byte>(key.begin(), key.end())));
        req.setValue(capnp::Data::Builder(kj::heapArray<kj::byte>(value.begin(), value.end())));
        req.setTtl(ttl);
        return req.send().then([LOG_CAPTURE](capnp::Response<Peer::SetDataResults> &&) {
            LOG_TRACE("got response");
        }, [LOG_CAPTURE](const kj::Exception &e) {
            LOG_DEBUG("Exception in request\n\t\t{}", e.getDescription().cStr());
        }).wait(client.getWaitScope());
    }
}

void PeerImpl::getDataItemsOnJoinHelper(std::optional<NodeInformation::Node> successorNode)
{
    LOG_GET
    capnp::EzRpcClient client{successorNode->getIp(), successorNode->getPort()};
    auto cap = client.getMain<Peer>();
    auto req = cap.getDataItemsOnJoinRequest();
    buildNode(req.getNewNode(), m_nodeInformation->getNode());

    /* Start RPC */
    req.send().then([LOG_CAPTURE, this](capnp::Response<Peer::GetDataItemsOnJoinResults> &&Response) {
        std::map<std::vector<uint8_t>, std::pair<std::vector<uint8_t>, uint16_t>> mapDataItemsToReturn;

        Response.getListOfDataItems().size();
        for (auto individualDataItem : Response.getListOfDataItems()) {
            const std::vector<uint8_t> key(individualDataItem.getKey().begin(), individualDataItem.getKey().end());
            std::vector<uint8_t> data(individualDataItem.getData().begin(), individualDataItem.getData().end());
            size_t expires_since_epoch = individualDataItem.getExpires();
            std::chrono::system_clock::time_point expires{std::chrono::seconds{expires_since_epoch}};
            m_nodeInformation->setDataExpires(key, data, expires);
        }
        LOG_TRACE("got response from GetDataItemsOnJoin");
    }, [LOG_CAPTURE](const kj::Exception &e) {
        LOG_DEBUG(e.getDescription().cStr());
    }).wait(client.getWaitScope());
}
