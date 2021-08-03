#include "api.h"
#include <utility>

using namespace api;

Api::Api(const Options &o):
    m_service(std::make_unique<asio::io_service>()),
    m_acceptor(std::make_unique<tcp::acceptor>(*m_service, tcp::endpoint(tcp::v4(), o.port)))
{
    LOG_GET
    m_isRunning = true;
    start_accept();
    m_serviceFuture = std::async(std::launch::async, [LOG_CAPTURE, this]() {
        LOG_TRACE("run()");
        while (m_isRunning) {
            try {
                m_service->run();
            }
            catch (const std::exception &e) {
                LOG_WARN("{}", e.what());
            }
        }
        LOG_TRACE("run() returned");
    });
}

Api::~Api()
{
    m_isRunning = false;
    SPDLOG_TRACE("closing acceptor");
    m_acceptor->close();
    SPDLOG_TRACE("stopping service");
    m_service->stop();
    SPDLOG_TRACE("awaiting service future");
    m_serviceFuture.get();

    for (const auto &connection: m_openConnections) {
        connection->close();
    }

    m_openConnections.clear();

    SPDLOG_TRACE("api stopped!");
}

void Api::start_accept()
{
    LOG_GET
    LOG_TRACE("");
    if (!m_isRunning) {
        LOG_INFO("acceptor is closed");
        return;
    }
    m_acceptor->async_accept([LOG_CAPTURE, this](const asio::error_code &error, tcp::socket socket) {
        start_accept();
        if (error)
            LOG_WARN("{}", error.message());
        else
            m_openConnections.push_back(std::make_unique<Connection>(std::move(socket), *this));
    });
    m_openConnections.erase(
        std::remove_if(m_openConnections.begin(), m_openConnections.end(), [](const std::unique_ptr<Connection> &connection) { return connection->isDone(); }),
        m_openConnections.end());
}
