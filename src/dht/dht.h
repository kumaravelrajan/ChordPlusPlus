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
        ~Dht() = default;
        Dht(const Dht &) = delete;
        Dht(Dht &&) = delete;

        void setApi(std::unique_ptr<api::Api> api);

    private:
        void mainLoop();

        std::vector<std::byte> onDhtPut(const api::Message_KEY_VALUE &m, std::atomic_bool &cancelled);
        std::vector<std::byte> onDhtGet(const api::Message_KEY &m, std::atomic_bool &cancelled);

        std::future<void> m_mainLoop;
        std::unique_ptr<api::Api> m_api;
    };

    void writeMessage(int fd);
    void printMessage(int fd);
}

#endif //DHT_DHT_H
