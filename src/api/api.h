#ifndef DHT_API_H
#define DHT_API_H

#include <future>
#include <memory>
#include <queue>
#include <optional>
#include <set>
#include <cstddef>
#include <asio.hpp>
#include <functional>
#include <type_traits>
#include <map>
#include "message_data.h"
#include "request.h"

namespace api
{
    struct Options
    {
        uint16_t port = 1234ull;
    };

    class Connection;
    class Api
    {
        using tcp = asio::ip::tcp;

        using request_handler_t = std::function<std::vector<uint8_t>(const MessageData &message_data, std::atomic_bool &cancelled)>;
        template<typename MSG_TYPE, std::enable_if_t<util::is_one_of_v<MSG_TYPE, Message_KEY, Message_DHT_PUT>, int> = 0>
        using request_handler_specific_t = std::function<std::vector<uint8_t>(const MSG_TYPE &message_data, std::atomic_bool &cancelled)>;

    public:
        explicit Api(const Options &o = {});
        ~Api();

        /**
         * @brief Set or unset request handler for specified request type. It is the responsibility of the requestHandler to periodically check the cancellation token, in order to keep shutdown times short.
         *
         * @tparam request_type
         * @param requestHandler
         */
        template<uint16_t request_type>
        void on(std::optional<request_handler_specific_t<message_type_from_int_t<request_type>>> requestHandler = {})
        {
            if (requestHandler)
                m_requestHandlers[request_type] = [r(requestHandler.value())](const MessageData &message_data, std::atomic_bool &cancelled) {
                    return r(dynamic_cast<const message_type_from_int_t<request_type> &>(message_data), cancelled);
                };
            else
                m_requestHandlers.erase(request_type);
        }

    private:
        void start_accept();

        std::unique_ptr<asio::io_service> m_service;
        std::unique_ptr<tcp::acceptor> m_acceptor;
        std::future<void> m_serviceFuture;

        std::vector<std::unique_ptr<Connection>> m_openConnections;

        std::map<uint16_t, request_handler_t> m_requestHandlers;

        std::atomic_bool m_isRunning = false;

        friend class Connection;
    };

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

        std::vector<uint8_t> m_data = std::vector<uint8_t>((1 << 16) - 1, 0);

        tcp::socket m_socket;
        const Api &m_api;

        std::atomic_bool m_done = false;

        std::deque<std::future<std::vector<uint8_t>>> m_handlerCalls;
        std::future<void> m_handlerCallManager;
        std::mutex m_handlerCallMutex;

        std::atomic_bool cancellation_token { false };
    };
} // namespace API

#endif //DHT_API_H
