//
// Created by maxib on 6/16/2021.
//

#ifndef DHT_PEER_H
#define DHT_PEER_H

#include <capnp/capability.h>
#include <peer.capnp.h>
#include <memory>

class NodeInformation;

namespace dht
{
    class PeerImpl : public Peer::Server
    {
        ::kj::Promise<void> getSuccessor(GetSuccessorContext context) override;

    public:
        void setNodeInformation(std::shared_ptr<NodeInformation>);

    private:
        std::shared_ptr<NodeInformation> m_nodeInformation;
    };
}


#endif //DHT_PEER_H
