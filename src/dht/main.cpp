#include <iostream>
#include <memory>

#include "Dht.h"

#include "api.h"
#include "NodeInformation.h"

int main()
{
    std::cout << "[DHT main] This is the main method for testing dht!" << std::endl;

    // fixme - Finding node id with sha1 hash
    NodeInformation N{};
    N.FindSha1Key(N.getMKey());

    {
        // The constructor of Dht starts mainLoop asynchronously.
        auto dht = std::make_unique<dht::Dht>(dht::Options{
            .address = "127.0.0.1",
            .port = 6969,
        });

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
