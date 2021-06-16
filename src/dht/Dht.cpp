#include "Dht.h"
#include <util.h>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include "person.capnp.h"

using namespace std::chrono_literals;

void dht::Dht::mainLoop()
{
    /*
     * This is where the dht will fix fingers, store and retrieve data, etc.
     * This needs to be thread-safe, and needs to be able to exit at any time.
     * (No blocking function calls in here)
     */

    std::cout << "[DHT] Main Loop Entered" << std::endl;

    while (m_dhtRunning)
        std::this_thread::sleep_for(500ms);

    std::cout << "[DHT] Exiting Main Loop" << std::endl;
}

void dht::Dht::setApi(std::unique_ptr<api::Api> api)
{
    // Destroy old api:
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

std::vector<std::byte> dht::Dht::onDhtPut(const api::Message_KEY_VALUE &message_data, std::atomic_bool &cancelled)
{
    // TODO: Store the request somewhere, and wait until the mainLoop has the answer

    // vvv can be removed
    std::cout << "[dht] DHT_PUT" << std::endl;
    for (uint8_t i{0}; !cancelled && i < 10; ++i)
        std::this_thread::sleep_for(1s);
    return message_data.m_bytes;
    // ^^^ can be removed
}

std::vector<std::byte> dht::Dht::onDhtGet(const api::Message_KEY &message_data, std::atomic_bool &cancelled)
{
    // TODO: Store the request somewhere, and wait until the mainLoop has the answer

    // vvv can be removed
    std::cout << "[dht] DHT_GET" << std::endl;
    for (uint8_t i{0}; !cancelled && i < 10; ++i)
        std::this_thread::sleep_for(1s);
    return message_data.m_bytes;
    // ^^^ can be removed
}
