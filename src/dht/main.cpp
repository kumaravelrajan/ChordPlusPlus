#include <iostream>
#include <memory>
#include <capnp/ez-rpc.h>

#include "peer.capnp.h"

#include "Dht.h"

#include "api.h"
#include "NodeInformation.h"

using namespace std::chrono_literals;

int main()
{
    // Note: Since DHT module is being initialized in dht-4.cpp, this main would not be called.
    // Note: There is no reason to get rid of this main.cpp, since it is ignored in the dht-4 target anyway.
    //       It is just used for testing.

    std::cout << "[DHT main] This is the main method for testing dht!" << std::endl;

    auto N = std::make_shared<NodeInformation>("127.0.0.1", 6969);

    {
        // The constructor of Dht starts mainLoop asynchronously.
        auto dht = std::make_unique<dht::Dht>(N, config::Configuration{});

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
