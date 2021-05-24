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

namespace API
{
    struct Options
    {
        uint16_t port = 1234ull;
    };

    class Connection;
    class Api
    {
        using tcp = asio::ip::tcp;

        using request_handler_t = std::function<std::vector<std::byte>(const MessageData &message_data)>;
        template<typename MSG_TYPE, std::enable_if_t<util::is_one_of_v<MSG_TYPE, Message_KEY, Message_KEY_VALUE>, int> = 0>
        using request_handler_specific_t = std::function<std::vector<std::byte>(const MSG_TYPE &message_data)>;

    public:
        explicit Api(const Options &o = {});
        ~Api();

        template<uint16_t request_type>
        void on(std::optional<request_handler_specific_t<message_type_from_int_t<request_type>>> requestHandler = {})
        {
            if (requestHandler)
                requestHandlers[request_type] = [r(requestHandler.value())](const MessageData &message_data) {
                    return r(dynamic_cast<const message_type_from_int_t<request_type> &>(message_data));
                };
            else
                requestHandlers.erase(request_type);
        }

    private:
        void start_accept();

        std::unique_ptr<asio::io_service> service;
        std::unique_ptr<tcp::acceptor> acceptor;
        std::future<void> serviceFuture;

        std::vector<std::unique_ptr<Connection>> openConnections;

        std::map<uint16_t, request_handler_t> requestHandlers;

        bool isRunning = false;

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

        void start_read();
        void close();
        void get();
        bool isDone();

    private:
        std::vector<uint8_t> data = std::vector<uint8_t>((1 << 16) - 1, 0);

        tcp::socket socket;
        const Api &api;

        bool done = false;
    };
} // namespace API

#endif //DHT_API_H
