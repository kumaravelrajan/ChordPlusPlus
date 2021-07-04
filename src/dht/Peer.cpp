#include "Peer.h"
#include "NodeInformation.h"
#include <kj/common.h>
#include <utility>
#include <capnp/ez-rpc.h>
#include <util.h>

using dht::PeerImpl;
using dht::Peer;
using dht::Node;


PeerImpl::PeerImpl(std::shared_ptr<NodeInformation> nodeInformation) :
    m_nodeInformation{std::move(nodeInformation)} {}

// Server methods

::kj::Promise<void> PeerImpl::getSuccessor(GetSuccessorContext context)
{
    std::cout << "[PEER] getSuccessorRequest" << std::endl;
    auto id_ = context.getParams().getId();
    NodeInformation::id_type id{};
    std::copy_n(id_.begin(),
                std::min(id_.size(), static_cast<size_t>(SHA_DIGEST_LENGTH)),
                id.begin());
    auto successor = getSuccessor(id);
    if (!successor) {
        context.getResults().getNode().setEmpty();
    } else {
        auto node = context.getResults().getNode().getValue();
        node.setIp(successor->getIp());
        node.setPort(successor->getPort());
        node.setId(capnp::Data::Builder(kj::heapArray<kj::byte>(successor->getId().begin(), successor->getId().end())));
    }
    return kj::READY_NOW;
}

::kj::Promise<void> PeerImpl::getPredecessor(GetPredecessorContext context)
{
    // std::cout << "[PEER] getPredecessorRequest" << std::endl;
    auto pred = m_nodeInformation->getPredecessor();
    if (pred) {
        auto node = context.getResults().getNode().getValue();
        node.setIp(pred->getIp());
        node.setPort(pred->getPort());
        node.setId(capnp::Data::Builder(kj::heapArray<kj::byte>(pred->getId().begin(), pred->getId().end())));
    } else {
        context.getResults().getNode().setEmpty();
    }
    return kj::READY_NOW;
}

::kj::Promise<void> PeerImpl::notify(NotifyContext context)
{
    std::cout << "[PEER] notifyRequest" << std::endl;

    auto node = nodeFromReader(context.getParams().getNode());

    if (!m_nodeInformation->getPredecessor() ||
        util::is_in_range_loop(
            node.getId(),
            m_nodeInformation->getPredecessor()->getId(),
            m_nodeInformation->getId(),
            false, false
        )) {
        std::cout << "[PEER.notify] update predecessor" << std::endl;
        m_nodeInformation->setPredecessor(node);
    }
    return kj::READY_NOW;
}

::kj::Promise<void> PeerImpl::getData(GetDataContext context)
{
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
    builder.setId(capnp::Data::Builder(kj::heapArray<kj::byte>(node.getId().begin(), node.getId().end())));
}

// Interface

std::optional<NodeInformation::Node> PeerImpl::getSuccessor(NodeInformation::id_type id)
{
    // std::cout << "[PEER.getSuccessor]" << std::endl;

    // If this node is requested
    if (m_nodeInformation->getPredecessor() &&
        util::is_in_range_loop(
            id,
            m_nodeInformation->getPredecessor()->getId(), m_nodeInformation->getId(),
            false, true
        )) {
        // std::cout << "[PEER.getSuccessor] requested this node" << std::endl;
        return m_nodeInformation->getNode();
    }

    // If next node is requested
    if (m_nodeInformation->getSuccessor() &&
        util::is_in_range_loop(
            id,
            m_nodeInformation->getId(), m_nodeInformation->getSuccessor()->getId(),
            false, true
        )) {
        // std::cout << "[PEER.getSuccessor] requested successor" << std::endl;
        return m_nodeInformation->getFinger(0);
    }

    // Otherwise, pass the request to the closest preceding finger
    auto closest_preceding = getClosestPreceding(id);

    if (!closest_preceding) {
        // std::cout << "[PEER.getSuccessor] didn't get closest preceding" << std::endl;
        return {};
    }

    capnp::EzRpcClient client{closest_preceding->getIp(), closest_preceding->getPort()};
    auto &waitScope = client.getWaitScope();
    auto cap = client.getMain<Peer>();
    auto req = cap.getSuccessorRequest();
    req.setId(capnp::Data::Builder{kj::heapArray<kj::byte>(id.begin(), id.end())});
    // std::cout << "[PEER.getSuccessor] before response" << std::endl;
    try {
        auto response = req.send().wait(waitScope).getNode();
        // std::cout << "[PEER.getSuccessor] got response" << std::endl;

        auto node = nodeFromReader(response);
        if (!node) {
            // std::cout << "[PEER.getSuccessor] closest preceding empty response" << std::endl;
        }
        return node;
    } catch (const kj::Exception &e) {
        std::cout << "[PEER.getSuccessor] Exception in request" << std::endl;
        return {};
    }
}

std::optional<NodeInformation::Node> PeerImpl::getClosestPreceding(NodeInformation::id_type id)
{
    // std::cout << "[PEER.getClosestPreceding]" << std::endl;
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
    if (node == m_nodeInformation->getNode()) {
        std::cout << "[PEER.getData] Get from this node" << std::endl;
        return m_nodeInformation->getData(key);
    } else {
        capnp::EzRpcClient client{node.getIp(), node.getPort()};
        auto &waitScope = client.getWaitScope();
        auto cap = client.getMain<Peer>();
        auto req = cap.getDataRequest();
        req.setKey(capnp::Data::Builder(kj::heapArray<kj::byte>(key.begin(), key.end())));
        std::cout << "[PEER.getData] before response" << std::endl;
        try {
            auto response = req.send().wait(waitScope).getData();
            std::cout << "[PEER.getData] got response" << std::endl;

            if (response.which() == Optional<capnp::Data>::EMPTY) {
                std::cout << "[PEER.getData] Empty response" << std::endl;
                return {};
            }
            std::cout << "[PEER.getData] Got data" << std::endl;
            return std::vector<uint8_t>{response.getValue().begin(), response.getValue().end()};

        } catch (const kj::Exception &e) {
            std::cout << "[PEER.getData] Exception in request" << std::endl;
            return {};
        }
    }
}

void PeerImpl::setData(
    const NodeInformation::Node &node,
    const std::vector<uint8_t> &key, const std::vector<uint8_t> &value,
    uint16_t ttl)
{
    auto ttls = ttl > 0
                ? std::chrono::seconds(ttl)
                : std::chrono::system_clock::duration::max();

    if (node == m_nodeInformation->getNode()) {
        std::cout << "[PEER.setData] Store in this node" << std::endl;
        m_nodeInformation->setData(key, value, ttls);
    } else {
        capnp::EzRpcClient client{node.getIp(), node.getPort()};
        auto &waitScope = client.getWaitScope();
        auto cap = client.getMain<Peer>();
        auto req = cap.setDataRequest();
        req.setKey(capnp::Data::Builder(kj::heapArray<kj::byte>(key.begin(), key.end())));
        req.setValue(capnp::Data::Builder(kj::heapArray<kj::byte>(value.begin(), value.end())));
        req.setTtl(ttl);
        std::cout << "[PEER.setData] before response" << std::endl;
        try {
            auto response = req.send().wait(waitScope);
            std::cout << "[PEER.setData] got response" << std::endl;
        } catch (const kj::Exception &e) {
            std::cout << "[PEER.setData] Exception in request" << std::endl;
        }
    }
}


void PeerImpl::create()
{
    std::cout << "[PEER.create]" << std::endl;
    m_nodeInformation->setPredecessor();
    m_nodeInformation->setSuccessor(m_nodeInformation->getNode());
}

void PeerImpl::join(const NodeInformation::Node &node)
{
    std::cout << "[PEER.join]" << std::endl;
    m_nodeInformation->setPredecessor();
    auto successor = getSuccessor(m_nodeInformation->getId());
    m_nodeInformation->setSuccessor(successor);
}

void PeerImpl::stabilize()
{
    auto &successor = m_nodeInformation->getSuccessor();
    if (!successor) {
        // std::cout << "[PEER.stabilize] no successor" << std::endl;
        return;
    }

    std::optional<NodeInformation::Node> predOfSuccessor{};

    capnp::EzRpcClient client{successor->getIp(), successor->getPort()};
    auto &waitScope = client.getWaitScope();
    auto cap = client.getMain<Peer>();
    auto req = cap.getPredecessorRequest();
    try {
        // std::cout << "[PEER.stabilize] before response" << std::endl;
        auto response = req.send().wait(waitScope).getNode();
        // std::cout << "[PEER.stabilize] got response" << std::endl;

        predOfSuccessor = nodeFromReader(response);
        if (!predOfSuccessor) {
            // std::cout << "[PEER.stabilize] closest preceding empty response" << std::endl;
        }
    } catch (const kj::Exception &e) {
        std::cout << "[PEER.stabilize] connection issue with successor" << std::endl;
        // Delete successor from finger table:
        successor = {};
        return;
    }

    if (predOfSuccessor && util::is_in_range_loop(
        predOfSuccessor->getId(), m_nodeInformation->getId(), successor->getId(),
        false, false
    )) {
        successor = predOfSuccessor;
        capnp::EzRpcClient client2{predOfSuccessor->getIp(), predOfSuccessor->getPort()};
        auto &waitScope2 = client2.getWaitScope();
        auto cap2 = client2.getMain<Peer>();
        auto req2 = cap2.notifyRequest();
        buildNode(req2.getNode(), m_nodeInformation->getNode());
        try {
            // std::cout << "[PEER.stabilize] before response" << std::endl;
            req2.send().wait(waitScope2);
            // std::cout << "[PEER.stabilize] got response" << std::endl;
        } catch (const kj::Exception &e) {
            // std::cout << "[PEER.stabilize] connection issue predecessor of successor" << std::endl;
        }
    }
}

void PeerImpl::fixFingers()
{
    // TODO

    m_nodeInformation->setFinger(
        nextFinger,
        getSuccessor(m_nodeInformation->getId() +
                     util::pow2<uint8_t, SHA_DIGEST_LENGTH>(nextFinger)));

    nextFinger = (nextFinger + 1) % NodeInformation::key_bits;
}

void PeerImpl::checkPredecessor()
{
    // TODO:

    if (!m_nodeInformation->getPredecessor())
        return;

    capnp::EzRpcClient client{
        m_nodeInformation->getPredecessor()->getIp(), m_nodeInformation->getPredecessor()->getPort()
    };
    auto &waitScope = client.getWaitScope();
    auto cap = client.getMain<Peer>();
    auto req = cap.getPredecessorRequest(); // This request doesn't matter, it is used as a ping
    try {
        // std::cout << "[PEER.checkPredecessor] before response" << std::endl;
        req.send().wait(waitScope).getNode();
        // std::cout << "[PEER.checkPredecessor] got response" << std::endl;
    } catch (const kj::Exception &e) {
        // std::cout << "[PEER.checkPredecessor] connection issue with predecessor" << std::endl;
        // Delete predecessor
        m_nodeInformation->setPredecessor();
    }
}
