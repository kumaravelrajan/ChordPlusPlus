#ifndef DHT_PEER_H
#define DHT_PEER_H

#include <vector>
#include <cstdint>
#include <kj/array.h>
#include <capnp/ez-rpc.h>
#include <capnp/message.h>
#include <peer.capnp.h>

namespace p2p
{
    class PeerImpl final : public Peer::Server
    {
    protected:
        kj::Promise<void> getSuccessor(GetSuccessorContext context) override
        {
            auto peerInfo = context.getResults().getPeerInfo();
            peerInfo.setKey(::capnp::Data::Builder(
                kj::heapArray<kj::byte>(std::initializer_list<kj::byte>{0x01, 0x23, 0x45, 0x67})));
            peerInfo.setIp("test ip");
            peerInfo.setPort(6969);
            return kj::READY_NOW;
        }
    };
}

#endif //DHT_PEER_H
