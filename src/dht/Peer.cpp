//
// Created by maxib on 6/16/2021.
//

#include "Peer.h"
#include "NodeInformation.h"
#include <kj/common.h>
#include <utility>

using dht::PeerImpl;


void PeerImpl::setNodeInformation(std::shared_ptr<NodeInformation> nodeInformation)
{ m_nodeInformation = std::move(nodeInformation); }

::kj::Promise<void> PeerImpl::getSuccessor(GetSuccessorContext context)
{
    std::cout << "getSuccessor called!" << std::endl;
    context.getResults().getPeerInfo().setIp("Some IP address");
    return kj::READY_NOW;
}