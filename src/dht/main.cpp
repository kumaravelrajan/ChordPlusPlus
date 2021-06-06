#include <iostream>
#include <memory>

#include "dht.h"

#include "api.h"
#include "NodeInformation.h"

int main()
{
    std::cout << "[DHT main] This is the main method for testing dht!" << std::endl;

    {
        // The constructor of Dht starts mainLoop asynchronously.
        auto dht = std::make_unique<dht::Dht>();
        std::cout << "[DHT main] destroying dht..." << std::endl;
    } // <- The destructor of Dht waits for mainLoop to exit.

    std::cout << "[DHT main] dht destroyed!" << std::endl;

    // fixme - Finding node id with sha1 hash
    NodeInformation N = NodeInformation();
    N.FindSha1Key(N.getMKey());
    return 0;
}
