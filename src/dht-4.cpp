#include <iostream>
#include <string>
#include <cxxopts.hpp>
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

    std::cout << args["config"].as<std::string>() << std::endl;

    return 0;
}
