#include "Peer.h"
#include "NodeInformation.h"
#include <kj/common.h>
#include <utility>
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
                std::min(id_.size(), static_cast<size_t>(SHA_DIGEST_LENGTH)),
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

    if (!m_nodeInformation->getPredecessor() ||
        util::is_in_range_loop(
            node.getId(),
            m_nodeInformation->getPredecessor()->getId(),
            m_nodeInformation->getId(),
            false, false
        )) {
        // std::cout << "[PEER.notify] update predecessor" << std::endl;
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

// Conversion

NodeInformation::Node PeerImpl::nodeFromReader(Node::Reader value)
{
    auto res_id_ = value.getId();
    NodeInformation::id_type res_id{};
    std::copy_n(res_id_.begin(),
                std::min(res_id_.size(), static_cast<size_t>(SHA_DIGEST_LENGTH)),
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

::kj::Promise <std::optional<NodeInformation::Node>> PeerImpl::getSuccessor(NodeInformation::id_type id)
{
    LOG_GET
    // If this node is requested
    if (m_nodeInformation->getPredecessor() &&
        util::is_in_range_loop(
            id,
            m_nodeInformation->getPredecessor()->getId(), m_nodeInformation->getId(),
            false, true
        )) {
        return std::optional<NodeInformation::Node>{m_nodeInformation->getNode()};
    }

    // If next node is requested
    if (m_nodeInformation->getSuccessor() &&
        util::is_in_range_loop(
            id,
            m_nodeInformation->getId(), m_nodeInformation->getSuccessor()->getId(),
            false, true
        )) {
        return m_nodeInformation->getFinger(0);
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
    return req.send().then([client = kj::mv(client)](capnp::Response <Peer::GetSuccessorResults> &&response) {
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
        return req.send().then([LOG_CAPTURE](capnp::Response <Peer::GetDataResults> &&response) {
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
        return req.send().then([LOG_CAPTURE](capnp::Response <Peer::SetDataResults> &&) {
            LOG_TRACE("got response");
        }, [LOG_CAPTURE](const kj::Exception &e) {
            LOG_DEBUG("Exception in request\n\t\t{}", e.getDescription().cStr());
        }).wait(client.getWaitScope());
    }
}

::kj::Promise<void> dht::PeerImpl::getDataItemsOnJoin(GetDataItemsOnJoinContext context)
{
    /* 1. Predecessor nodes needs to call some function GetDataItemsForNodeId() in NodeInformation.cpp file.
     * 2. The returned list from NodeInformation.cpp has to be returned as GetDataItemsOnJoinResults.*/

    /* Check if current node is predecessor of node in GetDataItemsOnJoinParams */
    ::capnp::Data::Reader keyOfNewNode = context.getParams().getNewNodeKey();
    std::vector<uint8_t> vKeyOfNewNode(keyOfNewNode.begin(), keyOfNewNode.end());
    std::array<uint8_t, SHA_DIGEST_LENGTH> arrKeyOfNewNode;
    std::copy_n(vKeyOfNewNode.begin(), SHA_DIGEST_LENGTH, arrKeyOfNewNode.begin());

    /*KJ_ASSERT(m_nodeInformation->getPredecessor()->getId() == arrKeyOfNewNode, "Wrong successor node.");*/

    if(m_nodeInformation->getPredecessor()->getId() == arrKeyOfNewNode){

        auto mapOfDataItemsForNewNode = m_nodeInformation->getDataItemsForNodeId(vKeyOfNewNode);

        if(mapOfDataItemsForNewNode){
            DataItem::Builder dataItemBuilder(nullptr);
            auto iter = mapOfDataItemsForNewNode->begin();

            for(int i = 0; i < mapOfDataItemsForNewNode->size(); ++i){
                dataItemBuilder.setKey(capnp::Data::Builder(kj::heapArray<kj::byte>(iter->first.begin(), iter->first.end())));
                dataItemBuilder.setData(capnp::Data::Builder(kj::heapArray<kj::byte>(iter->second.first.begin(), iter->second.first.end())));
                dataItemBuilder.setTtl(5/*Todo - iter->second.second.count()*/);
                std::advance(iter, 1);
            }
        }

        return kj::READY_NOW;
    }

    /* Todo - remove this */
    return kj::READY_NOW;
}

std::optional<PeerImpl::dataItem_type> PeerImpl::getDataItemsOnJoinHelper(std::optional<NodeInformation::Node> successorNode, std::shared_ptr<NodeInformation> &newNode)
{
    LOG_GET
    capnp::EzRpcClient client{ successorNode->getIp(), successorNode->getPort() };
    auto cap = client.getMain<Peer>();
    auto req = cap.getDataItemsOnJoinRequest();
    req.setNewNodeKey(capnp::Data::Builder(kj::heapArray<kj::byte>(newNode->getId().begin(), newNode->getId().end())));

    /* Start RPC */
    req.send().then([LOG_CAPTURE](capnp::Response<Peer::GetDataItemsOnJoinResults> &&Response) {
        LOG_TRACE("got response from GetDataItemsOnJoin");
        auto dataItemsToReturn = Response.getListOfDataItems();
        }, [LOG_CAPTURE, this](const kj::Exception &e) {
        // Delete predecessor
        LOG_ERROR("e");
        m_nodeInformation->setPredecessor();
    }).wait(client.getWaitScope());

    return PeerImpl::dataItem_type();
}
