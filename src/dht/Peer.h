#ifndef DHT_PEER_H
#define DHT_PEER_H

#include <capnp/capability.h>
#include <peer.capnp.h>
#include <memory>
#include <unordered_set>
#include <atomic>
#include "NodeInformation.h"

namespace dht
{
    class PeerImpl : public Peer::Server
    {
        using dataItem_type = std::map<std::vector<uint8_t>, std::pair<std::vector<uint8_t>, std::chrono::system_clock::time_point>>;
        /**
         * @brief This function returns the successor of the supplied key. Sending additional requests is also done here.
         */
        ::kj::Promise<void> getSuccessor(GetSuccessorContext context) override;

        /**
         * @brief Returns the closest preceding node of the supplied key, in addition to the direct successor of this node.
         */
        ::kj::Promise<void> getClosestPreceding(GetClosestPrecedingContext context) override;

        /**
         * @brief This function returns the predecessor of this node.
         */
        ::kj::Promise<void> getPredecessor(GetPredecessorContext context) override;

        /**
         * @brief This function notifies the node of a predecessor.
         */
        ::kj::Promise<void> notify(NotifyContext context) override;

        /**
         * @brief This function returns data if it is stored in this node.
         */
        ::kj::Promise<void> getData(GetDataContext context) override;

        /**
         * @brief This function stores data if this node is responsible for it.
         */
        ::kj::Promise<void> setData(SetDataContext context) override;

        /**
         * @brief This function returns data for which a node is responsible for from its successor.
         */
        ::kj::Promise<void> getDataItemsOnJoin(GetDataItemsOnJoinContext context) override;

        /**
         * @brief The new node asks the bootstrap node for its search puzzle.
         */
        ::kj::Promise<void> getPoWPuzzleOnJoin(GetPoWPuzzleOnJoinContext context) override;

        /**
         * @brief The new node sends its response to the PoW search puzzle to the bootstrap node.
         * The bootstrap node validates the response and sends the successor to the new node.
         */
        ::kj::Promise<void> sendPoWPuzzleResponseToBootstrapAndGetSuccessor(
            SendPoWPuzzleResponseToBootstrapAndGetSuccessorContext context) override;

        struct ClosestPrecedingPair
        {
            std::optional<NodeInformation::Node> closestPreceding{};
            std::optional<NodeInformation::Node> successor{};
        };

        /**
         * @brief
         * If node refers to this node, returns results for this node.
         * Otherwise makes request to the node in question.
         * @return Promise of maybe closest preceding node and/or direct successor of the supplied node.
         */
        ::kj::Promise<ClosestPrecedingPair>
        getClosestPrecedingHelper(const NodeInformation::Node &node, const NodeInformation::id_type &id,
                                  std::shared_ptr<std::unordered_set<NodeInformation::Node, NodeInformation::Node::Node_hash>> distrusted);

    public:
        enum class GetSuccessorMethod
        {
            PASS_ON, LOCAL
        };

        explicit PeerImpl(std::shared_ptr<NodeInformation>, GetSuccessorMethod = GetSuccessorMethod::LOCAL);

        static NodeInformation::Node nodeFromReader(Node::Reader node);
        static std::optional<NodeInformation::Node> nodeFromReader(Optional<Node>::Reader node);
        static void buildNode(Node::Builder builder, const NodeInformation::Node &node);
        static void buildNode(Optional<Node>::Builder builder, const std::optional<NodeInformation::Node> &node);
        static NodeInformation::id_type idFromReader(capnp::Data::Reader id);
        static capnp::Data::Builder buildId(const NodeInformation::id_type &id);

        ::kj::Promise<std::optional<NodeInformation::Node>> getSuccessor(NodeInformation::id_type id);
        std::optional<NodeInformation::Node> getClosestPreceding(NodeInformation::id_type id);

        std::optional<std::vector<uint8_t>> getData(const NodeInformation::Node &node, const std::vector<uint8_t> &key);
        void setData(const NodeInformation::Node &node,
                     const std::vector<uint8_t> &key, const std::vector<uint8_t> &value,
                     uint16_t ttl);

        void getDataItemsOnJoinHelper(std::optional<NodeInformation::Node> successorNode);

    private:
        GetSuccessorMethod m_getSuccessorMethod;
        std::shared_ptr<NodeInformation> m_nodeInformation;
    };
}


#endif //DHT_PEER_H
