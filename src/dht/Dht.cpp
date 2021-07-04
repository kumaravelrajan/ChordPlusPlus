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
        m_nodeInformation->getIp(),
        m_nodeInformation->getPort()
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

    while (true) {
        if (!m_dhtRunning) break;
        if (!m_nodeInformation->getSuccessor()) {
            if (m_nodeInformation->getBootstrapNodeAddress()) {
                getPeerImpl().join(*m_nodeInformation->getBootstrapNodeAddress());
            } else {
                getPeerImpl().create();
            }
        }

        if (!m_dhtRunning) break;
        getPeerImpl().stabilize();
        if (!m_dhtRunning) break;
        getPeerImpl().fixFingers();
        if (!m_dhtRunning) break;
        getPeerImpl().checkPredecessor();
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
        [this](const api::Message_DHT_PUT &m, std::atomic_bool &cancelled) {
            return onDhtPut(m, cancelled);
        });
    m_api->on<util::constants::DHT_GET>(
        [this](const api::Message_DHT_GET &m, std::atomic_bool &cancelled) {
            return onDhtGet(m, cancelled);
        });
}

std::optional<NodeInformation::Node> Dht::getSuccessor(NodeInformation::id_type key)
{
    auto response = getPeerImpl().getSuccessor(key);
    return response;
}

std::vector<uint8_t> Dht::onDhtPut(const api::Message_DHT_PUT &message_data, std::atomic_bool &cancelled)
{
    // TODO

    std::cout << "[DHT] DHT_PUT" << std::endl;

    std::cout
        << "[DHT.put] size:        " << std::to_string(message_data.m_header.size) << std::endl
        << "[DHT.put] type:        " << std::to_string(message_data.m_header.msg_type) << std::endl
        << "[DHT.put] ttl:         " << std::to_string(message_data.m_headerExtend.ttl) << std::endl
        << "[DHT.put] replication: " << std::to_string(message_data.m_headerExtend.replication) << std::endl
        << "[DHT.put] reserved:    " << std::to_string(message_data.m_headerExtend.reserved) << std::endl;

    // Hashing received key to convert it into length of 20 bytes
    std::string sKey{message_data.key.begin(), message_data.key.end()};
    NodeInformation::id_type finalHashedKey = NodeInformation::hash_sha1(sKey);

    auto ttl = message_data.m_headerExtend.ttl > 0
               ? std::chrono::seconds(message_data.m_headerExtend.ttl)
               : std::chrono::system_clock::duration::max();

    std::cout << "[DHT.put] getSuccessor" << std::endl;
    auto successor = getSuccessor(finalHashedKey);
    if (successor) {
        std::cout << "[DHT.put] Successor got: \"" << successor.value().getIp() << "\"" << std::endl;
        getPeerImpl().setData(*successor, message_data.key, message_data.value, message_data.m_headerExtend.ttl);
    } else {
        std::cout << "[DHT.put] No Successor got!" << std::endl;
    }

    for (uint8_t i{0}; !cancelled && i < 10; ++i)
        std::this_thread::sleep_for(1s);
    return message_data.m_bytes;
}

std::vector<uint8_t> Dht::onDhtGet(const api::Message_DHT_GET &message_data, std::atomic_bool &cancelled)
{
    // TODO

    std::cout << "[DHT] DHT_GET" << std::endl;

    std::cout
        << "[DHT.get] size:        " << std::to_string(message_data.m_header.size) << std::endl
        << "[DHT.get] type:        " << std::to_string(message_data.m_header.msg_type) << std::endl;

    // Hashing received key to convert it into length of 20 bytes
    std::string sKey{message_data.key.begin(), message_data.key.end()};
    NodeInformation::id_type finalHashedKey = NodeInformation::hash_sha1(sKey);

    std::cout << "[DHT.get] getSuccessor" << std::endl;
    auto successor = getSuccessor(finalHashedKey);
    std::optional<std::vector<uint8_t>> response{};
    if (successor) {
        std::cout << "[DHT.get] Successor got: " << successor->getIp() << ":" << successor->getPort() << std::endl;
        response = getPeerImpl().getData(*successor, message_data.key);
    } else {
        std::cout << "[DHT.get] No Successor got!" << std::endl;
    }

    if (response)
        return *response;

    for (uint8_t i{0}; !cancelled && i < 10; ++i)
        std::this_thread::sleep_for(1s);
    return message_data.m_bytes;
}
