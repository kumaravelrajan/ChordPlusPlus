#include <iostream>
#include <string>
#include <cxxopts.hpp>
#include <config.h>
#include <entry.h>
#include <memory>
#include <logging/centralLogControl.h>

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
    spdlog::set_default_logger(async_file_logger);

    spdlog::flush_every(5s);

    SPDLOG_DEBUG("spdlog initialized.");
}

int main(int argc, char *argv[])
{
    cxxopts::Options options("dht-4", "DHT module for the VoidPhone project");
    options.add_options()
        (
            "c,config", "Configuration file path",
            cxxopts::value<std::string>()->default_value("./config.ini")
        )
        ("h,help", "Print usage")
        (
            "n,testNodeAmount",
            "Create multiple nodes on localhost for testing.\n0 means no extra nodes.",
            cxxopts::value<uint64_t>()->default_value("0")
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
        );
    auto args = options.parse(argc, argv);

    if (args.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    std::cout << "Config path: " << args["config"].as<std::string>() << std::endl;

    auto conf = config::parseConfigFile(args["config"].as<std::string>());

    // Get user input nodes to create
    if (args.count("testNodeAmount")) {
        conf.extra_debug_nodes = args["testNodeAmount"].as<uint64_t>();
    }

    // Initialize spdlog
    InitSpdlog(args["logMode"].as<int>(), args["logOutput"].as<std::string>());

    SPDLOG_DEBUG("Config path: {}", args["config"].as<std::string>());

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
