#include <iostream>
#include <string>
#include <cxxopts.hpp>
#include <config.h>
#include <entry.h>

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
            "t,testCreateNodes",
            "Create multiple nodes on localhost for testing.\n0 for no extra nodes.",
            cxxopts::value<int>()->default_value("0")
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

    std::cout
        << "p2p_address           : " << conf.p2p_address << "\n"
        << "p2p_port              : " << conf.p2p_port << "\n"
        << "api_address           : " << conf.api_address << "\n"
        << "api_port              : " << conf.api_port << "\n"
        << "bootstrapNode_address : " << conf.bootstrapNode_address << "\n"
        << "bootstrapNode_port    : " << conf.bootstrapNode_port << std::endl;

    int ret = 0;
    {
        entry::Entry e{conf};
        ret = e.mainLoop();
    }

    return ret;
}
