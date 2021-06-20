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
    m_nodeInformation{std::move(nodeInformation)}
{}

// Server methods

::kj::Promise<void> PeerImpl::getSuccessor(GetSuccessorContext context)
{
    std::cout << "[PEER] getSuccessorRequest" << std::endl;
    auto id_ = context.getParams().getId();
    NodeInformation::id_type id{};
    std::copy_n(id_.begin(),
                std::min(id_.size(), static_cast<unsigned long>(SHA_DIGEST_LENGTH)),
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
    std::cout << "[PEER] getPredecessorRequest" << std::endl;
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
            m_nodeInformation->getMSha1NodeId(),
            false, false
        )) {
        std::cout << "[PEER.notify] update predecessor" << std::endl;
        m_nodeInformation->setPredecessor(node);
    }
}

// Conversion

NodeInformation::Node PeerImpl::nodeFromReader(Node::Reader value)
{
    auto res_id_ = value.getId();
    NodeInformation::id_type res_id{};
    std::copy_n(res_id_.begin(),
                std::min(res_id_.size(), static_cast<unsigned long>(SHA_DIGEST_LENGTH)),
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
    std::cout << "[PEER] getSuccessor" << std::endl;

    // If requested is this node
    if (m_nodeInformation->getPredecessor() &&
        util::is_in_range_loop(
            id,
            m_nodeInformation->getPredecessor()->getId(), m_nodeInformation->getMSha1NodeId(),
            false, true
        )) {
        std::cout << "[PEER] requested this node" << std::endl;
        return m_nodeInformation->getNode();
    }

    // If requested is next node
    if (m_nodeInformation->getFinger(0) &&
        util::is_in_range_loop(
            id,
            m_nodeInformation->getMSha1NodeId(), m_nodeInformation->getFinger(0)->getId(),
            false, true
        )) {
        std::cout << "[PEER] requested successor" << std::endl;
        return m_nodeInformation->getFinger(0);
    }

    // Otherwise, pass the request to the closest preceding finger
    auto closest_preceding = getClosestPreceding(id);

    if (!closest_preceding) {
        std::cout << "[PEER] didn't get closest preceding" << std::endl;
        return {};
    }

    capnp::EzRpcClient client{closest_preceding->getIp(), closest_preceding->getPort()};
    auto &waitScope = client.getWaitScope();
    auto cap = client.getMain<Peer>();
    auto req = cap.getSuccessorRequest();
    req.setId(capnp::Data::Builder{kj::heapArray<kj::byte>(id.begin(), id.end())});
    std::cout << "[PEER] before response" << std::endl;
    auto response = req.send().wait(waitScope).getNode();
    std::cout << "[PEER] got response" << std::endl;

    auto node = nodeFromReader(response);
    if (!node) {
        std::cout << "[PEER] closest preceding empty response" << std::endl;
    }
    return node;
}

std::optional<NodeInformation::Node> PeerImpl::getClosestPreceding(NodeInformation::id_type id)
{
    std::cout << "[PEER] getClosestPreceding" << std::endl;
    for (size_t i = NodeInformation::key_bits; i >= 1ull; --i) {
        if (util::is_in_range_loop(
            m_nodeInformation->getFinger(i - 1)->getId(), m_nodeInformation->getMSha1NodeId(), id,
            false, false
        ))
            return m_nodeInformation->getFinger(i - 1);
    }
    return {};
}

void PeerImpl::create()
{
    m_nodeInformation->setPredecessor();
    m_nodeInformation->setSuccessor(m_nodeInformation->getNode());
}

void PeerImpl::join(const Node &node)
{
    m_nodeInformation->setPredecessor();
    auto successor = getSuccessor(m_nodeInformation->getMSha1NodeId());
    m_nodeInformation->setSuccessor(successor);
}

void PeerImpl::stabilize()
{
    auto successor = m_nodeInformation->getSuccessor();
    if (!successor) {
        std::cout << "[PEER.stabilize] no successor" << std::endl;
        return;
    }

    capnp::EzRpcClient client{successor->getIp(), successor->getPort()};
    auto &waitScope = client.getWaitScope();
    auto cap = client.getMain<Peer>();
    auto req = cap.getPredecessorRequest();
    std::cout << "[PEER.stabilize] before response" << std::endl;
    auto response = req.send().wait(waitScope).getNode();
    std::cout << "[PEER.stabilize] got response" << std::endl;

    auto node = nodeFromReader(response);
    if (!node) {
        std::cout << "[PEER] closest preceding empty response" << std::endl;
    }

    if (util::is_in_range_loop(
        node->getId(), m_nodeInformation->getMSha1NodeId(), m_nodeInformation->getSuccessor()->getId(),
        false, false
    )) {
        capnp::EzRpcClient client2{node->getIp(), node->getPort()};
        auto &waitScope2 = client.getWaitScope();
        auto cap2 = client.getMain<Peer>();
        auto req2 = cap.notifyRequest();
        buildNode(req2.getNode(), m_nodeInformation->getNode());
        std::cout << "[PEER.stabilize] before response" << std::endl;
        auto response2 = req.send().wait(waitScope).getNode();
        std::cout << "[PEER.stabilize] got response" << std::endl;
    }
}

void PeerImpl::fixFingers()
{
    // TODO
}

void PeerImpl::checkPredecessor()
{
    // TODO
}
