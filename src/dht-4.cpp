#include <iostream>
#include <string>
#include <cxxopts.hpp>
#include <config.h>

#include <iostream>
#include <memory>

#include "Dht.h"

#include "api.h"
#include "NodeInformation.h"

using namespace std::chrono_literals;

void StartDHT(int dhtNodesToCreate, uint16_t portInLocalhost)
{
    std::cout << "[DHT main] This is the main method for testing dht!" << std::endl;
    std::cout << "============================================================================================";
    std::cout << "ThreadId = " << std::this_thread::get_id << std::endl << "Port = " << portInLocalhost << std::endl;
    std::cout << "============================================================================================";
    auto N = std::make_shared<NodeInformation>();

    {
        // The constructor of Dht starts mainLoop asynchronously.
        auto dht = std::make_unique<dht::Dht>(N);

        dht->setApi(std::make_unique<api::Api>(api::Options{
            .port= portInLocalhost,
        }));

        // Wait for input:
        std::cin.get();

        std::cout << "[DHT main] destroying dht..." << std::endl;
    } // <- The destructor of Dht waits for mainLoop to exit.

    std::cout << "[DHT main] dht destroyed!" << std::endl;

    std::cin.get();
}

int main(int argc, char *argv[])
{
    int dhtNodesToCreate = 0;

    cxxopts::Options options("dht-4", "DHT module for the VoidPhone project");
    options.add_options()
        (
            "c,config", "Configuration file path",
            cxxopts::value<std::string>()->default_value("./config.ini")
        )
        ("h,help", "Print usage")

        ("t,testCreateNodes",
             "Create multiple nodes on localhost for testing.",
             cxxopts::value<int>()->default_value("1"))
        ;
    auto args = options.parse(argc, argv);

    if (args.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    // Get user input nodes to create
    if (args.count("testCreateNodes")) {
        dhtNodesToCreate = args["testCreateNodes"].as<int>();
    }

    std::cout << "Config path: " << args["config"].as<std::string>() << std::endl;

    const auto conf = config::parseConfigFile(args["config"].as<std::string>());

    std::cout
        << "p2p_address: " << conf.p2p_address << "\n"
        << "p2p_port:    " << conf.p2p_port << "\n"
        << "api_address: " << conf.api_address << "\n"
        << "api_port:    " << conf.api_port << "\n"
        << "bootstrapNode_address:    " << conf.bootstrapNode_address << "\n"
        << "bootstrapNode_port:    " << conf.bootstrapNode_port << std::endl;

    // Note: Starting DHT (and implicitly API as well) at port portForDhtNode.

    /*
     * Reason for ListOfFutures
    A std::future object returned by std::async and launched with std::launch::async policy, blocks on destruction until the task that was launched has completed.
    If std::future returned by std::async is not stored in a variable, it is destroyed at the end of the statement with std::async and as such, main cannot continue until the task is done.
    Hence, storing the std::future object in ListOfFutures where its lifetime will be extended to the end of main and we get the behavior we want.
     */

    std::vector<std::future<void>> ListOfFutures;

    // 0-1023 are system ports. Avoiding those ports.
    int portForDhtNode = 1024;
    for(int i = 0; i < dhtNodesToCreate; i++)
    {
        ListOfFutures.push_back(std::async(std::launch::async, StartDHT, dhtNodesToCreate, portForDhtNode));
        ++portForDhtNode;

        // Note - Temp solution to space out thread creation so that there is no race condition for resources like std::cout

        if(!(i == (dhtNodesToCreate - 1)))
            std::this_thread::sleep_for(30s);
    }

    // Wait for input
    std::cin.get();

    // TODO: enter command-line interface

    return 0;
}
