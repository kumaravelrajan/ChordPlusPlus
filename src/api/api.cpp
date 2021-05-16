#include "api.h"
#include <iostream>
#include <chrono>
#include <memory>
#include <cstdlib>
#include <util.h>

using namespace API;

Api::Api(const Options &o):
    service(std::make_unique<asio::io_service>()),
    acceptor(std::make_unique<tcp::acceptor>(*service, tcp::endpoint(tcp::v4(), 1234)))
{
    isRunning = true;
    start_accept();
    service_future = std::async(std::launch::async, [this]() {
        std::cout << "[API] run()" << std::endl;
        while (isRunning) {
            try {
                std::cout << "[API] run()" << std::endl;
                service->run();
            }
            catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;
            }
        }
        std::cout << "[API] run() returned" << std::endl;
    });
}

Api::~Api()
{
    isRunning = false;
    std::cout << "[API] closing acceptor" << std::endl;
    acceptor->close();
    std::cout << "[API] stopping service" << std::endl;
    service->stop();
    std::cout << "[API] awaiting service future" << std::endl;
    service_future.get();

    for(const auto &connection : openConnections) {
        connection->close();
    }

    // TODO: stop open connection handlers

    std::cout << "[API] stopped!" << std::endl;
}

void Api::start_accept()
{
    std::cout << "[API] start_accept()" << std::endl;
    if (!isRunning) {
        std::cout << "[API.start_accept] acceptor is closed" << std::endl;
        return;
    }
    acceptor->async_accept([this](const asio::error_code &error, tcp::socket socket) {
        std::cout << "async accept happened" << std::endl;
        start_accept();
        openConnections.push_back(std::make_unique<Connection>(error, std::move(socket)));
    });
}
