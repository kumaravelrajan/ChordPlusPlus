#include <iostream>
#include <string>
#include <cxxopts.hpp>
#include <config.h>
#include <memory>

#include "Dht.h"
#include "api.h"
#include "NodeInformation.h"
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using namespace std::chrono_literals;

void StartDHT(size_t dhtNodesToCreate, uint16_t startPortForDhtNodes, const config::Configuration &conf)
{
    auto N = std::make_shared<NodeInformation>();
    std::vector<std::unique_ptr<dht::Dht>> vListOfDhtNodes = {};
    std::vector<std::shared_ptr<NodeInformation>> vListOfNodeInformationObj = {};
    uint16_t portinLocalHost = startPortForDhtNodes;

    for (size_t i = 0; i < dhtNodesToCreate; i++) {
        {
            vListOfNodeInformationObj.push_back(std::make_shared<NodeInformation>("127.0.0.1", portinLocalHost));

            // Set bootstrap node details parsed from config file
            vListOfNodeInformationObj[i]->setBootstrapNodeAddress(
                NodeInformation::Node(conf.bootstrapNode_address, conf.bootstrapNode_port)
            );

            // The constructor of Dht starts mainLoop asynchronously.
            vListOfDhtNodes.push_back(std::make_unique<dht::Dht>(vListOfNodeInformationObj[i]));

            ++portinLocalHost;

            vListOfDhtNodes[i]->setApi(std::make_unique<api::Api>(api::Options{
                .port= portinLocalHost,
            }));

            SPDLOG_INFO("\n==================================================================================\n"
                "1. Node number {} \n2. Node port {} \n3. Node API port {}\n"
                "==================================================================================\n", i,
                   vListOfNodeInformationObj[i]->getPort(), portinLocalHost);

            ++portinLocalHost;
        }
    }

    // Wait for input:
    std::cin.get();

    std::cout << "[DHT main] destroying dht..." << std::endl;
    vListOfDhtNodes.clear();
    std::cout << "[DHT main] dht destroyed!" << std::endl;
}

void SetSpdlogParams()
{
    spdlog::init_thread_pool(8192, 1);

    auto stderr_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt >();
    stderr_sink->set_level(spdlog::level::warn);

    auto basic_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt> ("./async_log.txt", true);
    basic_file_sink->set_level(spdlog::level::trace);

    std::vector<spdlog::sink_ptr> sinks {stderr_sink, basic_file_sink};

    auto async_file_logger = std::make_shared<spdlog::async_logger>("async_file_logger", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    async_file_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %^[%l] [%s::%!()-#%#] %v%$");
    spdlog::set_default_logger(async_file_logger);

    spdlog::flush_every(std::chrono::seconds(5));

    SPDLOG_ERROR("spdlog initialized.");
}

int main(int argc, char *argv[])
{
    SetSpdlogParams();

    size_t dhtNodesToCreate = 0;

    cxxopts::Options options("dht-4", "DHT module for the VoidPhone project");
    options.add_options()
        (
            "c,config", "Configuration file path",
            cxxopts::value<std::string>()->default_value("./config.ini")
        )
        ("h,help", "Print usage")
        (
            "t,testCreateNodes",
            "Create multiple nodes on localhost for testing.",
            cxxopts::value<size_t>()->default_value("1")
        );
    auto args = options.parse(argc, argv);

    if (args.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    // Get user input nodes to create
    dhtNodesToCreate = args["testCreateNodes"].as<size_t>();

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
    uint16_t startPortForDhtNodes = 1024;
    StartDHT(dhtNodesToCreate, startPortForDhtNodes, conf);

    // Wait for input
    std::cin.get();

    // TODO: enter command-line interface

    return 0;
}
