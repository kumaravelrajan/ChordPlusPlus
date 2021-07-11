#include "entry.h"
#include <iostream>
#include <spdlog/fmt/fmt.h>

using entry::Entry;

Entry::Entry(const config::Configuration &conf)
{
    uint16_t dht_port = conf.p2p_port;
    uint16_t api_port = conf.api_port;

    for (size_t i = 0; i <= conf.extra_debug_nodes; i++) {
        {
            vListOfNodeInformationObj.push_back(std::make_shared<NodeInformation>("127.0.0.1", dht_port));

            // Set bootstrap node details parsed from config file
            vListOfNodeInformationObj[i]->setBootstrapNodeAddress(
                NodeInformation::Node(conf.bootstrapNode_address, conf.bootstrapNode_port)
            );

            // The constructor of Dht starts mainLoop asynchronously.
            vListOfDhtNodes.push_back(std::make_unique<dht::Dht>(vListOfNodeInformationObj[i]));

            vListOfDhtNodes[i]->setApi(std::make_unique<api::Api>(api::Options{
                .port= api_port,
            }));

            std::cout << "=================================================================================="
                      << std::endl;
            std::cout << fmt::format(
                ""
                "1. Node number   : {}\n"
                "2. Node port     : {}\n"
                "3. Node API port : {}",
                i, dht_port, api_port
            ) << std::endl;
            std::cout << "=================================================================================="
                      << std::endl;

            ++dht_port;
            ++api_port;
        }
    }
}

Entry::~Entry()
{
    std::cout << "[ENTRY] exiting..." << std::endl;
    vListOfDhtNodes.clear();
    vListOfNodeInformationObj.clear();
    std::cout << "[ENTRY] Dht Stopped." << std::endl;
}

int Entry::mainLoop()
{
    // TODO: command line interface
    std::cin.get();
    return 0;
}

