#include "Dht.h"
#include <util.h>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/ez-rpc.h>

using dht::Dht;
using namespace std::chrono_literals;

void Dht::runServer()
{
    auto peerImpl = kj::heap<PeerImpl>(m_nodeInformation);
    m_peerImpl = *peerImpl;
    ::capnp::EzRpcServer peerServer{
        std::move(peerImpl),
        m_nodeInformation->getMIp(),
        m_nodeInformation->getMPort()
    };
    auto &waitScope = peerServer.getWaitScope();

    m_executor.emplace(kj::getCurrentThreadExecutor());

    // It seems like promises are atomic in the event loop. So no *actual* asynchronous execution... Threads it is then
    auto f = std::async(std::launch::async, [this]() {
        mainLoop();
    });

    while (m_dhtRunning) {
        waitScope.poll();
    }

    m_peerImpl.reset();
}

void Dht::mainLoop()
{
    /*
     * This is where the dht will fix fingers, store and retrieve data, etc.
     * This needs to be thread-safe, and needs to be able to exit at any time.
     * (No blocking function calls in here, at least not for too long)
     */

    std::cout << "[DHT] Main Loop Entered" << std::endl;

    // TODO: get bootstrap node
    // TODO: getPeerImpl().join(entryNode);

    while (true) {
        if (!m_dhtRunning) break;
        m_executor.value().get().executeSync([this] { getPeerImpl().stabilize(); });
        if (!m_dhtRunning) break;
        m_executor.value().get().executeSync([this] { getPeerImpl().fixFingers(); });
        if (!m_dhtRunning) break;
        m_executor.value().get().executeSync([this] { getPeerImpl().checkPredecessor(); });
        if (!m_dhtRunning) break;

        // Wait one second
        std::this_thread::sleep_for(1s);
        // std::cout << "Main loop, m_dhtRunning: " << m_dhtRunning << std::endl;
    }

    std::cout << "[DHT] Exiting Main Loop" << std::endl;
}

void Dht::setApi(std::unique_ptr<api::Api> api)
{
    // Destroy old api (This is in two statements for easier debugging):
    m_api = nullptr;
    // Move in new api:
    m_api = std::move(api);

    // Set Request handlers:
    m_api->on<util::constants::DHT_PUT>(
        [this](const api::Message_KEY_VALUE &m, std::atomic_bool &cancelled) {
            return onDhtPut(m, cancelled);
        });
    m_api->on<util::constants::DHT_GET>(
        [this](const api::Message_KEY &m, std::atomic_bool &cancelled) {
            return onDhtGet(m, cancelled);
        });
}

std::optional<NodeInformation::Node> Dht::getSuccessor(NodeInformation::id_type key)
{
    auto response = getPeerImpl().getSuccessor(key);
    return response;
}

std::vector<uint8_t> Dht::onDhtPut(const api::Message_KEY_VALUE &message_data, std::atomic_bool &cancelled)
{
    // TODO

    std::cout << "[DHT] DHT_PUT" << std::endl;

    // Hashing received key to convert it into length of 20 bytes
    std::stringstream ssUInt_AsStream;
    for(int i = 0; i < message_data.key.size(); ++i)
    {
        ssUInt_AsStream << message_data.key[i];
    }
    std::string sKey = ssUInt_AsStream.str();
    NodeInformation::id_type finalHashedKey = NodeInformation::FindSha1Key(sKey);

    std::cout << "[DHT] getSuccessor" << std::endl;
    auto successor = getSuccessor(
        finalHashedKey
    );
    if (successor)
        std::cout << "[DHT] Successor got: \"" << successor.value().getIp() << "\"" << std::endl;
    else
        std::cout << "[DHT] No Successor got!" << std::endl;


    for (uint8_t i{0}; !cancelled && i < 10; ++i)
        std::this_thread::sleep_for(1s);
    return message_data.m_bytes;
}

std::vector<uint8_t> Dht::onDhtGet(const api::Message_KEY &message_data, std::atomic_bool &cancelled)
{
    // TODO

    // vvv can be removed
    std::cout << "[DHT] DHT_GET" << std::endl;
    for (uint8_t i{0}; !cancelled && i < 10; ++i)
        std::this_thread::sleep_for(1s);
    return message_data.m_bytes;
    // ^^^ can be removed
}
