#ifndef DHT_DHT_H
#define DHT_DHT_H

#include <api.h>
#include <future>
#include <utility>
#include <vector>
#include <cstddef>
#include <memory>
#include <message_data.h>
#include <shared_mutex>
#include <peer.capnp.h>
#include <exception>
#include <stdexcept>
#include "Peer.h"
#include "NodeInformation.h"

namespace dht
{
    class Dht
    {
    public:
        explicit Dht(std::shared_ptr<NodeInformation> nodeInformation, config::Configuration conf) :
            m_nodeInformation(std::move(nodeInformation)),
            m_mainLoop(std::async(std::launch::async, [this]() { runServer(); })),
            m_conf(std::move(conf)) {}
        ~Dht()
        {
            m_dhtCancelled = true;
            m_api = nullptr;
            m_mainLoop.wait(); // This happens after the destructor anyway, but this way it is clearer

            if (m_replicationFuture.valid()) {
                m_replicationFuture.wait();
            }
        };
        Dht(const Dht &) = delete;
        Dht(Dht &&) = delete;

        /**
         * The api is used to receive requests.
         * @param api - unique, transfers ownership
         */
        void setApi(std::unique_ptr<api::Api> api);

    private:
        void runServer();

        /**
         * This is where the actual work happens.
         * mainLoop is called asynchronously from the constructor of Dht.
         * It needs to worry about stopping itself.
         */
        void mainLoop();

        void create();
        void join(const NodeInformation::Node &node);
        void stabilize();
        void fixFingers();
        void checkPredecessor();


        [[nodiscard]] std::optional<NodeInformation::Node> getSuccessor(NodeInformation::id_type key);
        std::vector<uint8_t> onDhtPut(const api::Message_DHT_PUT &m, std::atomic_bool &cancelled);
        std::vector<uint8_t> onDhtGet(const api::Message_KEY &m, std::atomic_bool &cancelled);
        std::vector<uint8_t> onDhtPutKeyIsHashOfData(const api::Message_DHT_PUT_KEY_IS_HASH_OF_DATA &message_data,
                                                          std::atomic_bool &cancelled);
        std::vector<uint8_t> onDhtGetKeyIsHashOfData(const api::Message_DHT_GET_KEY_IS_HASH_OF_DATA &message_data,
                                                     std::atomic_bool &cancelled);

        std::shared_ptr<NodeInformation> m_nodeInformation;
        std::future<void> m_mainLoop;
        std::future<void> m_replicationFuture;
        std::unique_ptr<api::Api> m_api;
        std::atomic_bool m_dhtCancelled{false};
        std::atomic_bool m_mainLoopExited{false};
        std::optional<std::reference_wrapper<PeerImpl>> m_peerImpl;
        std::optional<std::reference_wrapper<const kj::Executor>> m_executor;
        std::atomic<size_t> nextFinger{0};
        const config::Configuration m_conf;

        // Getters

        /**
         * @throw std::bad_optional_access
         * @return PeerImpl instance
         */
        auto &getPeerImpl()
        {
            return m_peerImpl.value().get();
        }
    };
}

#endif //DHT_DHT_H
