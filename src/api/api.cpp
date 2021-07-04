#include "api.h"
#include "message_data.h"
#include <iostream>
#include <chrono>
#include <memory>
#include <cstdlib>
#include <utility>
#include <algorithm>
#include <util.h>

using namespace api;

Api::Api(const Options &o):
    m_service(std::make_unique<asio::io_service>()),
    m_acceptor(std::make_unique<tcp::acceptor>(*m_service, tcp::endpoint(tcp::v4(), o.port)))
{
    m_isRunning = true;
    start_accept();
    m_serviceFuture = std::async(std::launch::async, [this]() {
        std::cout << "[API] run()" << std::endl;
        while (m_isRunning) {
            try {
                m_service->run();
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
    m_isRunning = false;
    std::cout << "[API] closing acceptor" << std::endl;
    m_acceptor->close();
    std::cout << "[API] stopping service" << std::endl;
    m_service->stop();
    std::cout << "[API] awaiting service future" << std::endl;
    m_serviceFuture.get();

    for (const auto &connection: m_openConnections) {
        connection->close();
    }

    m_openConnections.clear();

    std::cout << "[API] stopped!" << std::endl;
}

void Api::start_accept()
{
    std::cout << "[API] start_accept()" << std::endl;
    if (!m_isRunning) {
        std::cout << "[API.start_accept] acceptor is closed" << std::endl;
        return;
    }
    m_acceptor->async_accept([this](const asio::error_code &error, tcp::socket socket) {
        start_accept();
        if (error)
            std::cerr << "[API.async_accept] " << error.message() << std::endl;
        else
            m_openConnections.push_back(std::make_unique<Connection>(std::move(socket), *this));
    });
    m_openConnections.erase(
        std::remove_if(m_openConnections.begin(), m_openConnections.end(), [](const std::unique_ptr<Connection> &connection) { return connection->isDone(); }),
        m_openConnections.end());
}
