#ifndef DHT_PEER_H
#define DHT_PEER_H

#include <capnp/capability.h>
#include <peer.capnp.h>
#include <memory>
#include "NodeInformation.h"

namespace dht
{
    class PeerImpl : public Peer::Server
    {
        /**
         * @brief This function returns the successor of the supplied key. Sending additional requests is also done here.
         */
        ::kj::Promise<void> getSuccessor(GetSuccessorContext context) override;

        /**
         * @brief This function returns the predecessor of this node.
         */
        ::kj::Promise<void> getPredecessor(GetPredecessorContext context) override;

    public:
        explicit PeerImpl(std::shared_ptr<NodeInformation>);

        std::optional<NodeInformation::Node> getSuccessor(NodeInformation::id_type id);

        void create();
        void join(const Node &node);
        void stabilize();
        void notify(const Node &node);
        void fixFingers();
        void checkPredecessor();

    private:
        std::shared_ptr<NodeInformation> m_nodeInformation;
        capnp::Orphanage m_orphanage{};
    };
}


#endif //DHT_PEER_H
