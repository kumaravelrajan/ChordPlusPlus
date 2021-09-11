#include <iostream>
#include <string>
#include <cxxopts.hpp>
#include <config.h>
#include <entry.h>
#include <memory>
#include <optional>
#include <centralLogControl.h>

using namespace std::literals;

void InitSpdlog(const int &consoleLogLevel, const std::string &logfilePath)
{
    spdlog::init_thread_pool(8192, 1);

    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    if (consoleLogLevel >= 0 && consoleLogLevel < spdlog::level::n_levels) {
        stdout_sink->set_level((spdlog::level::level_enum) consoleLogLevel);
    } else {
        stdout_sink->set_level(spdlog::level::warn);
    }

    stdout_sink->set_color_mode(spdlog::color_mode::always);

    auto basic_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logfilePath, true);
    basic_file_sink->set_level(spdlog::level::trace);

    std::vector<spdlog::sink_ptr> sinks{stdout_sink, basic_file_sink};

    auto async_file_logger = std::make_shared<spdlog::async_logger>("async_file_logger", sinks.begin(), sinks.end(),
                                                                    spdlog::thread_pool(),
                                                                    spdlog::async_overflow_policy::block);
    async_file_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %^[%l] [%s::%!()-#%#] %v%$");
    async_file_logger->set_level(spdlog::level::debug);
    spdlog::set_default_logger(async_file_logger);

    spdlog::flush_every(5s);

    SPDLOG_DEBUG("spdlog initialized.");
}

int main(int argc, char *argv[])
{
    std::optional<std::string> startup_script{};
    std::optional<std::string> configPath{};

    cxxopts::Options options("dht-4", "DHT module for the VoidPhone project");
    options.add_options()
        (
            "c,config", "Configuration file path",
            cxxopts::value(configPath)->default_value("./config.ini")
        )
        ("h,help", "Print usage")
        (
            "n,node-amount",
            "Create multiple nodes on localhost for testing.\n0 means no nodes at all. Default is 1.",
            cxxopts::value<uint64_t>()->default_value("1")
        )
        (
            "l,logMode",
            "Set console log mode visibility (int): 0 - trace; 1 - debug; 2 - info; 3 - warning; 4 - error; 5 - critical; 6 - off"
            "\n(Complete logs are present in ./async-log.txt)",
            cxxopts::value<int>()->default_value("3")
        )
        (
            "o,logOutput",
            "Specify path for log file.\nDefault: ./log.txt",
            cxxopts::value<std::string>()->default_value("./log.txt")
        )
        (
            "s,startupScript",
            "Specify path for startup script.",
            cxxopts::value(startup_script)
        )
        (
            "d, POWdifficulty",
            "Specify difficulty to be set for Proof of work algorithm. \nUsage: -d {NUMBER_OF_LEADING_ZEROES <= 160}",
            cxxopts::value<uint8_t>()
        )
        (
            "r, replicationLimit",
            "Specify replication limit for the same data item on one node",
            cxxopts::value<uint8_t>()
        );
    auto args = options.parse(argc, argv);

    if (args.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    auto conf = configPath ? config::parseConfigFile(*configPath) : config::Configuration{};

    conf.startup_script = conf.startup_script ? conf.startup_script : startup_script;

    // Get user input nodes to create
    if (args.count("node-amount")) {
        conf.node_amount = args["node-amount"].as<uint64_t>();
    }

    // Get difficulty level for PoW
    if (args.count("POWdifficulty")) {
        conf.PoW_Difficulty = args["POWdifficulty"].as<uint8_t>();
    }

    // Get replication limit for the same data item on each node
    if(args.count("replicationLimit")){
        conf.defaultReplicationLimit = args["replicationLimit"].as<uint8_t>();
    }

    // Initialize spdlog
    InitSpdlog(args["logMode"].as<int>(), args["logOutput"].as<std::string>());

    if (configPath)
        SPDLOG_DEBUG("Config path: {}", *configPath);

    SPDLOG_INFO(
        "\n\t=================================================================================="
        "\n\t\tp2p_address: {}"
        "\n\t\tp2p_port: {}"
        "\n\t\tapi_address: {}"
        "\n\t\tapi_port: {}"
        "\n\t\tbootstrapNode_address: {}"
        "\n\t\tbootstrapNode_port: {}"
        "\n\t==================================================================================",
        conf.p2p_address, conf.p2p_port, conf.api_address, conf.api_port, conf.bootstrapNode_address,
        conf.bootstrapNode_port
    );

    int ret;
    {
        entry::Entry e{conf};
        ret = e.mainLoop();
    }
    return ret;
}
