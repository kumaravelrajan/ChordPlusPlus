#include <centralLogControl.h>
#include <util.h>
#include "connection.h"
#include "api.h"

using api::Connection;

Connection::Connection(tcp::socket &&sock, const Api &api) :
    m_socket(std::move(sock)),
    m_api(api)
{
    start_read();

    m_handlerCallManager = std::async(std::launch::async, [this]() {
        using namespace std::chrono_literals;

        while (m_socket.is_open()) {

            if (!m_handlerCalls.empty() && m_handlerCalls[0].wait_for(0s) == std::future_status::ready) {

                m_handlerCallMutex.lock();
                auto bytes = m_handlerCalls.front().get();
                m_handlerCalls.pop_front();
                m_handlerCallMutex.unlock();

                if (m_socket.is_open() && !bytes.empty()) {
                    m_socket.write_some(asio::buffer(bytes));
                }
            }

            std::this_thread::sleep_for(100ms);
        }
        m_done = true;
    });
}

void Connection::start_read() // NOLINT
{
    LOG_GET
    if (!m_socket.is_open()) {
        LOG_INFO("disconnected!");
        return;
    }

    asio::async_read(
        m_socket,
        asio::buffer(m_header),
        [LOG_CAPTURE, this](const asio::error_code &error, std::size_t length) { // NOLINT
            finish_read();

            if (error) {
                LOG_INFO("{}", error.message());
                return;
            }
        }
    );
}

void Connection::finish_read() // NOLINT
{
    LOG_GET
    if (!m_socket.is_open()) {
        LOG_INFO("disconnected!");
        return;
    }

    MessageHeader header(reinterpret_cast<MessageHeader::MessageHeaderRaw &>(m_header[0]));

    asio::async_read(
        m_socket,
        asio::buffer(m_data, header.size - sizeof(MessageHeader::MessageHeaderRaw)),
        [LOG_CAPTURE, this, header(std::move(header))](const asio::error_code &error, std::size_t length) { // NOLINT
            std::vector<uint8_t> bytes(m_header.size() + length);
            std::copy(m_header.begin(), m_header.end(), bytes.begin());
            std::copy(
                m_data.begin(),
                m_data.begin() + static_cast<long>(length),
                bytes.begin() + static_cast<long>(m_header.size())
            );

            start_read();

            if (error) {
                LOG_INFO("{}", error.message());
                return;
            }

            if (length > 0ull) {
                LOG_INFO("Got Data!\n{}\n", util::hexdump(bytes, 16, true, true));

                std::unique_ptr<Request> request;
                try {
                    request = std::make_unique<Request>(header, bytes);
                }
                catch (const std::exception &e) {
                    LOG_WARN("Exception thrown when parsing request:\n\t\t{}", e.what());
                    return;
                }
                if (m_socket.is_open()) {
                    auto t = request->getData()->m_header.msg_type;
                    if (m_api.m_requestHandlers.contains(t)) {
                        m_handlerCallMutex.lock();
                        m_handlerCalls.push_back(std::async(std::launch::async, [this, request(std::move(request))]() {
                            return m_api.m_requestHandlers.find(request->getData()->m_header.msg_type)->second(
                                *(request->getData<>()), cancellation_token);
                        }));
                        m_handlerCallMutex.unlock();
                    }
                }
            }
        }
    );
}

void Connection::close()
{
    m_socket.close();
}

bool Connection::isDone() const
{
    return m_done;
}

Connection::~Connection()
{
    cancellation_token = true;
}