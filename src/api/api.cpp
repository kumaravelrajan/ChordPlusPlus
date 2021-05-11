#include "api.h"
#include <iostream>
#include <chrono>

using namespace API;

Api::Api()
{
    isRunning = true;
    server = std::async(std::launch::async, [this]() { this->run(); });
    std::cout << "[API] started!" << std::endl;
}

Api::~Api()
{
    std::cout << "[API] stopping thread" << std::endl;
    isRunning = false;

    // TODO: stop open connection handlers


    server.get();
    std::cout << "[API] awaited" << std::endl;
}

void Api::run()
{
    using namespace std::chrono_literals;
    // TODO: create socket, listen, poll, and dispatch connection handlers
    std::cout << "[API] run() called" << std::endl;
    while(isRunning) {
        std::this_thread::sleep_for(666ms);
    }
    std::cout << "[API] isRunning=false" << std::endl;
}
