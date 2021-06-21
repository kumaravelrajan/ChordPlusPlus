#include <iostream>
#include <memory>

#include "Dht.h"

#include "api.h"
#include "NodeInformation.h"

using namespace std::chrono_literals;

int main()
{
    std::cout << "[DHT main] This is the main method for testing dht!" << std::endl;

    auto N = std::make_shared<NodeInformation>();
    N->setMIp("127.0.0.1");
    N->setMPort(6969);

    {
        // The constructor of Dht starts mainLoop asynchronously.
        auto dht = std::make_unique<dht::Dht>(N);

        dht->setApi(std::make_unique<api::Api>(api::Options{
            .port= 1234,
        }));

        // Wait for input:
        std::cin.get();

        std::cout << "[DHT main] destroying dht..." << std::endl;
    } // <- The destructor of Dht waits for mainLoop to exit.

    std::cout << "[DHT main] dht destroyed!" << std::endl;
    return 0;
}
