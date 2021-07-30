#include <iostream>
#include <string>
#include <cxxopts.hpp>
#include <config.h>
#include <memory>

#include "Dht.h"
#include "api.h"
#include "NodeInformation.h"
#include "centralLogControl.h"

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

            SPDLOG_INFO("\n\t\t=================================================================================="
                "\n\t\t1. Node number {} \n\t\t2. Node port {} \n\t\t3. Node API port {}"
                "\n\t\t==================================================================================", i,
                   vListOfNodeInformationObj[i]->getPort(), portinLocalHost);

            ++portinLocalHost;
        }
    }

    // Wait for input:
    std::cin.get();

    SPDLOG_INFO("[DHT main] destroying dht...");
    vListOfDhtNodes.clear();
    SPDLOG_INFO("[DHT main] dht destroyed!");
}

void InitSpdlog(int &consoleLogLevel)
{
    spdlog::init_thread_pool(8192, 1);

    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();

    if(consoleLogLevel >= spdlog::level::trace || consoleLogLevel <= spdlog::level::off) {
        stdout_sink->set_level((spdlog::level::level_enum)consoleLogLevel);
    } else {
        stdout_sink->set_level(spdlog::level::warn);
    }

    stdout_sink->set_color_mode(spdlog::color_mode::always);

    auto basic_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt> ("./async_log.txt", true);
    basic_file_sink->set_level(spdlog::level::trace);

    std::vector<spdlog::sink_ptr> sinks {stdout_sink, basic_file_sink};

    auto async_file_logger = std::make_shared<spdlog::async_logger>("async_file_logger", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    async_file_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %^[%l] [%s::%!()-#%#] %v%$");
    spdlog::set_default_logger(async_file_logger);

    spdlog::flush_every(std::chrono::seconds(5));

    SPDLOG_INFO("spdlog initialized.");
}

int main(int argc, char *argv[])
{
    size_t dhtNodesToCreate = 0;
    int consoleLogLevel = 0;

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
        )
        (
            "l,logMode",
            "Set console log mode visibility (int): 1 - trace; 2 - debug; 3 - info; 4 - warning; 5 - error; 6 - critical; 7 - off\n(Complete logs are present in ./async-log.txt)",
            cxxopts::value<int>()->default_value("3")
        );
    auto args = options.parse(argc, argv);

    if (args.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    // Get user input nodes to create
    dhtNodesToCreate = args["testCreateNodes"].as<size_t>();

    // Get user set console log level
    consoleLogLevel = args["logMode"].as<int>();

    // Initialize spdlog
    InitSpdlog(consoleLogLevel);

    SPDLOG_INFO("Config path: {}", args["config"].as<std::string>());

    const auto conf = config::parseConfigFile(args["config"].as<std::string>());

    SPDLOG_INFO("\n\t\t=================================================================================="
        "\n\t\tp2p_address: {}"
        "\n\t\tp2p_port: {}"
        "\n\t\tapi_address: {}"
        "\n\t\tapi_port: {}"
        "\n\t\tbootstrapNode_address: {}"
        "\n\t\tbootstrapNode_port: {}"
        "\n\t\t==================================================================================", conf.p2p_address, conf.p2p_port, conf.api_address, conf.api_port, conf.bootstrapNode_address, conf.bootstrapNode_port);

    // Note: Starting DHT (and implicitly API as well) at port portForDhtNode.

    // 0-1023 are system ports. Avoiding those ports.
    uint16_t startPortForDhtNodes = 1024;
    StartDHT(dhtNodesToCreate, startPortForDhtNodes, conf);

    // Wait for input
    std::cin.get();

    // TODO: enter command-line interface

    return 0;
}
