#include "api.h"
#include <iostream>
#include <chrono>
#include <memory>
#include <cstdlib>
#include <utility>
#include <algorithm>
#include <util.h>

using namespace API;

Api::Api(const Options &o):
    service(std::make_unique<asio::io_service>()),
    acceptor(std::make_unique<tcp::acceptor>(*service, tcp::endpoint(tcp::v4(), o.port)))
{
    isRunning = true;
    start_accept();
    serviceFuture = std::async(std::launch::async, [this]() {
        std::cout << "[API] run()" << std::endl;
        while (isRunning) {
            try {
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
    serviceFuture.get();

    for (const auto &connection: openConnections) {
        connection->close();
    }

    for (const auto &connection: openConnections) {
        connection->get();
    }

    // TODO: stop open connection handlers

    std::cout << "[API] stopped!" << std::endl;
}

void Api::setRequestHandler(std::optional<request_handler_t> handler)
{
    this->requestHandler = std::move(handler);
}

void Api::start_accept()
{
    std::cout << "[API] start_accept()" << std::endl;
    if (!isRunning) {
        std::cout << "[API.start_accept] acceptor is closed" << std::endl;
        return;
    }
    acceptor->async_accept([this](const asio::error_code &error, tcp::socket socket) {
        start_accept();
        if (error)
            std::cerr << "[API.async_accept] " << error.message() << std::endl;
        else
            openConnections.push_back(std::make_unique<Connection>(error, std::move(socket), *this));
    });
    openConnections.erase(
        std::remove_if(openConnections.begin(), openConnections.end(), [](const std::unique_ptr<Connection> &connection) { return connection->isDone(); }),
        openConnections.end());
}
