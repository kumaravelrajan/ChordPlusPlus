#include <iostream>
#include <string>
#include <cxxopts.hpp>
#include <config.h>
#include <entry.h>
#include <memory>

#include "Dht.h"
#include "api.h"
#include "NodeInformation.h"
#include "centralLogControl.h"

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
            cxxopts::value<uint64_t>()->default_value("1")
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

    std::cout << "Config path: " << args["config"].as<std::string>() << std::endl;

    auto conf = config::parseConfigFile(args["config"].as<std::string>());

    // Get user input nodes to create
    if (args.count("testCreateNodes")) {
        conf.extra_debug_nodes = args["testCreateNodes"].as<uint64_t>();
    }

    // Get user set console log level
    consoleLogLevel = args["logMode"].as<int>();

    // Initialize spdlog
    InitSpdlog(consoleLogLevel);

    SPDLOG_INFO("Config path: {}", args["config"].as<std::string>());

    SPDLOG_INFO("\n\t\t=================================================================================="
        "\n\t\tp2p_address: {}"
        "\n\t\tp2p_port: {}"
        "\n\t\tapi_address: {}"
        "\n\t\tapi_port: {}"
        "\n\t\tbootstrapNode_address: {}"
        "\n\t\tbootstrapNode_port: {}"
        "\n\t\t==================================================================================", conf.p2p_address, conf.p2p_port, conf.api_address, conf.api_port, conf.bootstrapNode_address, conf.bootstrapNode_port);

    int ret;
    {
        entry::Entry e{conf};
        ret = e.mainLoop();
    }

    return ret;
}
