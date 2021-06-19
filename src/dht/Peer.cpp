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
    context.getResults().getPeerInfo().setIp("Some IP address");
    context.getResults().getPeerInfo().setPort(2021);
    context.getResults().getPeerInfo().setId(
        capnp::Data::Builder(kj::arr<kj::byte>(
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20)));
    return kj::READY_NOW;
}

NodeInformation::Node PeerImpl::getSuccessor(kj::Array<kj::byte> id)
{
    // TODO: Either return this Node, or send request to one of the finger entries
    std::cout << "[PEER] getSuccessor" << std::endl;
    capnp::EzRpcClient client{"127.0.0.1", 6969};
    auto &waitScope = client.getWaitScope();
    auto cap = client.getMain<Peer>();
    auto req = cap.getSuccessorRequest();
    req.setKey(capnp::Data::Builder{id});
    std::cout << "[PEER] before response" << std::endl;
    auto response = req.send().wait(waitScope).getPeerInfo();
    std::cout << "[PEER] got response" << std::endl;
    auto res_id_ = response.getId();
    std::cout << "[PEER] got id" << std::endl;
    std::array<uint8_t, SHA_DIGEST_LENGTH> res_id{};
    std::copy_n(res_id_.begin(),
                std::min(res_id_.size(), static_cast<unsigned long>(SHA_DIGEST_LENGTH)),
                res_id.begin());
    NodeInformation::Node node(
        response.getIp(),
        response.getPort(),
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
    std::cout << "[PEER] stabilize" << std::endl;
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
