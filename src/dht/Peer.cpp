#include "Peer.h"
#include "NodeInformation.h"
#include <kj/common.h>
#include <utility>
#include <capnp/ez-rpc.h>

using dht::PeerImpl;
using dht::Peer;
using dht::Node;


PeerImpl::PeerImpl(std::shared_ptr<NodeInformation> nodeInformation) :
    m_nodeInformation{std::move(nodeInformation)}
{}

::kj::Promise<void> PeerImpl::getSuccessor(GetSuccessorContext context)
{
    std::cout << "getSuccessor called!" << std::endl;
    context.getResults().getNode().getValue().setIp("Some IP address");
    context.getResults().getNode().getValue().setPort(2021);
    context.getResults().getNode().getValue().setId(
        capnp::Data::Builder(kj::arr<kj::byte>(
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20)));
    return kj::READY_NOW;
}

::kj::Promise<void> PeerImpl::getPredecessor(GetPredecessorContext context)
{
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

std::optional<NodeInformation::Node> PeerImpl::getSuccessor(NodeInformation::id_type id)
{
    // TODO: Either return this Node, or send request to one of the finger entries
    std::cout << "[PEER] getSuccessor" << std::endl;

    if (id == m_nodeInformation->getMSha1NodeId()) {
        std::cout << "[PEER] requested this node" << std::endl;
        return m_nodeInformation->getNode();
    }

    /*
     * if id in (this.id, fingers[i].id] (make sure to make this a circular comparison)
     */
    // TODO: if id is between this and successor, then use successor

    // TODO: otherwise, use closest preceding successor and forward the request

    capnp::EzRpcClient client{"127.0.0.1", 6969};
    auto &waitScope = client.getWaitScope();
    auto cap = client.getMain<Peer>();
    auto req = cap.getSuccessorRequest();
    req.setKey(capnp::Data::Builder{kj::heapArray<kj::byte>(id.begin(), id.end())});
    std::cout << "[PEER] before response" << std::endl;
    auto response = req.send().wait(waitScope).getNode();
    std::cout << "[PEER] got response" << std::endl;

    // empty
    if (response.which() == Optional<Node>::EMPTY) return {};

    auto value = response.getValue();
    auto res_id_ = value.getId();
    std::cout << "[PEER] got id" << std::endl;
    NodeInformation::id_type res_id{};
    std::copy_n(res_id_.begin(),
                std::min(res_id_.size(), static_cast<unsigned long>(SHA_DIGEST_LENGTH)),
                res_id.begin());
    NodeInformation::Node node(
        value.getIp(),
        value.getPort(),
        res_id
    );
    std::cout << "[PEER] got ip and port" << std::endl;
    return node;
}

void PeerImpl::create()
{
    // TODO
}

void PeerImpl::join(const Node &node)
{
    // TODO
}

void PeerImpl::stabilize()
{
    // TODO
}

void PeerImpl::notify(const Node &node)
{
    // TODO
}

void PeerImpl::fixFingers()
{
    // TODO
}

void PeerImpl::checkPredecessor()
{
    // TODO
}
