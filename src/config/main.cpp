#include <iostream>
#include "config.h"

int main()
{
    std::cout << "This is the main method for testing config!" << std::endl;

    const auto conf = config::parseConfigFile(R"(C:\Users\maxib\Documents\config.ini)");

    std::cout
        << "p2p_address: " << conf.p2p_address << "\n"
        << "p2p_port:    " << conf.p2p_port << "\n"
        << "api_address: " << conf.api_address << "\n"
        << "api_port:    " << conf.api_port << std::endl;

    return 0;
}