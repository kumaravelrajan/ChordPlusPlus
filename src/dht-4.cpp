#include <iostream>
#include <string>
#include <cxxopts.hpp>
#include <config.h>
#include "api.h"

int main(int argc, char *argv[])
{
    cxxopts::Options options("dht-4", "DHT module for the VoidPhone project");
    options.add_options()
        (
            "c,config", "Configuration file path",
            cxxopts::value<std::string>()->default_value("./config.ini")
        )
        ("h,help", "Print usage");
    auto args = options.parse(argc, argv);

    if (args.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    std::cout << "Config path: " << args["config"].as<std::string>() << std::endl;

    const auto conf = config::parseConfigFile(args["config"].as<std::string>());

    std::cout
        << "p2p_address: " << conf.p2p_address << "\n"
        << "p2p_port:    " << conf.p2p_port << "\n"
        << "api_address: " << conf.api_address << "\n"
        << "api_port:    " << conf.api_port << std::endl;

    // TODO: start API
    // TODO: start DHT
    // TODO: enter command-line interface

    return 0;
}
