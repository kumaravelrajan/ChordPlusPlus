#ifndef DHT_DHT_H
#define DHT_DHT_H

#include <api.h>
#include <future>

namespace dht
{
    class Dht
    {
    public:
        Dht() : m_mainLoop(std::async(std::launch::async, [this]() { mainLoop(); }))
        {}

        ~Dht() = default;

        Dht(const Dht &) = delete;

        Dht(Dht &&) = delete;

    private:
        void mainLoop();

        std::future<void> m_mainLoop;
    };

    void writeMessage(int fd);

    void printMessage(int fd);
}

#endif //DHT_DHT_H
