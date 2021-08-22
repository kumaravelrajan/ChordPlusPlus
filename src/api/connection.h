#ifndef DHT_CONNECTION_H
#define DHT_CONNECTION_H

#include <asio.hpp>
#include <queue>
#include <future>
#include <vector>
#include <cstdint>
#include <atomic>
#include <mutex>
#include "message_data.h"

namespace api
{
    class Api;

    class Connection
    {
        using tcp = asio::ip::tcp;

    public:
        Connection() = delete;
        Connection(tcp::socket &&socket, const Api &api);
        Connection(const Connection &other) = delete;
        Connection(Connection &&other) = delete;

        Connection &operator=(const Connection &other) = delete;
        Connection &operator=(Connection &&other) = delete;

        void close();
        [[nodiscard]] bool isDone() const;

        ~Connection();

    private:
        void start_read();
        void finish_read();

        std::array<uint8_t, sizeof(api::MessageHeader::MessageHeaderRaw)> m_header;
        std::vector<uint8_t> m_data = std::vector<uint8_t>((1 << 16) - 1, 0);

        tcp::socket m_socket;
        const Api &m_api;

        std::atomic_bool m_done = false;

        std::deque<std::future<std::vector<uint8_t>>>
            m_handlerCalls;
        std::future<void> m_handlerCallManager;
        std::mutex m_handlerCallMutex;

        std::atomic_bool cancellation_token{false};
    };
}

#endif //DHT_CONNECTION_H
