#ifndef DHT_DHT_H
#define DHT_DHT_H

#include <api.h>
#include <future>
#include <vector>
#include <cstddef>
#include <memory>
#include <message_data.h>

namespace dht
{
    class Dht
    {
    public:
        Dht() :
            m_mainLoop(std::async(std::launch::async, [this]() { mainLoop(); }))
        {}
        ~Dht()
        {
            m_api = nullptr;
            m_dhtRunning = false;
            m_mainLoop.wait(); // This happens after the destructor anyway, but this way it is clearer
        };
        Dht(const Dht &) = delete;
        Dht(Dht &&) = delete;

        /**
         * The api is used to receive requests.
         * @param api - unique, transfers ownership
         */
        void setApi(std::unique_ptr<api::Api> api);

    private:
        /**
         * This is where the actual work happens.
         * mainLoop is called asynchronously from the constructor of Dht.
         * It needs to worry about stopping itself.
         */
        void mainLoop();

        std::vector<std::byte> onDhtPut(const api::Message_KEY_VALUE &m, std::atomic_bool &cancelled);
        std::vector<std::byte> onDhtGet(const api::Message_KEY &m, std::atomic_bool &cancelled);

        std::future<void> m_mainLoop;
        std::unique_ptr<api::Api> m_api;
        std::atomic_bool m_dhtRunning{true};
    };

    // vvv can be removed
    void writeMessage(int fd);
    void printMessage(int fd);
    // ^^^ can be removed
}

#endif //DHT_DHT_H
