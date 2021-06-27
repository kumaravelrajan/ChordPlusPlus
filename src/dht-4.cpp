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

void StartDHT(int dhtNodesToCreate, uint16_t startPortForDhtNodes, const config::Configuration &conf)
{
    auto N = std::make_shared<NodeInformation>();
    std::vector<std::unique_ptr<dht::Dht>> vListOfDhtNodes = {};
    std::vector<std::shared_ptr<NodeInformation>> vListOfNodeInformationObj = {};
    uint16_t portinLocalHost = startPortForDhtNodes;

    for(int i = 0; i < dhtNodesToCreate; i++)
    {
        {
            vListOfNodeInformationObj.push_back(std::make_shared<NodeInformation>(portinLocalHost));

            // Set bootstrap node details parsed from config file
            vListOfNodeInformationObj[i]->setBootstrapNodeAddress(std::make_pair(conf.bootstrapNode_address, conf.bootstrapNode_port));

            // The constructor of Dht starts mainLoop asynchronously.
           vListOfDhtNodes.push_back(std::make_unique<dht::Dht>(vListOfNodeInformationObj[i]));

            ++portinLocalHost;

            vListOfDhtNodes[i]->setApi(std::make_unique<api::Api>(api::Options{
                .port= portinLocalHost,
            }));

            std::cout << "==================================================================================" <<std::endl;
            printf("1. Node number %d \n2. Node port %d \n3. Node API port %d\n", i, vListOfNodeInformationObj[i]->getMPort(), portinLocalHost);
            std::cout << "=================================================================================="<<std::endl;

            ++portinLocalHost;

        } // <- The destructor of Dht waits for mainLoop to exit.
    }

    // Wait for input:
    std::cin.get();

    std::cout << "[DHT main] destroying dht..." << std::endl;

    std::cout << "[DHT main] dht destroyed!" << std::endl;
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

    // 0-1023 are system ports. Avoiding those ports.
    int startPortForDhtNodes = 1024;
    StartDHT(dhtNodesToCreate, startPortForDhtNodes, conf);

    // Wait for input
    std::cin.get();

    // TODO: enter command-line interface

    return 0;
}
