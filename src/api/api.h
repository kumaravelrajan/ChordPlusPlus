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
        using request_handler_t = std::function<std::vector<std::byte>(const Request &request)>;

    public:
        explicit Api(const Options &o = {});
        ~Api();

        void setRequestHandler(std::optional<request_handler_t> requestHandler);

    private:
        void start_accept();

        std::unique_ptr<asio::io_service> service;
        std::unique_ptr<tcp::acceptor> acceptor;
        std::future<void> serviceFuture;

        std::vector<std::unique_ptr<Connection>> openConnections;

        std::optional<request_handler_t> requestHandler;

        bool isRunning = false;

        friend class Connection;
    };

    class Connection
    {
        using tcp = asio::ip::tcp;

    public:
        Connection() = delete;
        Connection(asio::error_code error, tcp::socket &&socket, const Api &api);
        Connection(const Connection &other) = delete;
        Connection(Connection &&other) = delete;

        Connection &operator=(const Connection &other) = delete;
        Connection &operator=(Connection &&other) = delete;

        void close();
        void get();
        bool isDone();

    private:
        tcp::socket socket;
        std::future<void> session;
        const Api &api;

        bool done = false;
    };
} // namespace API

#endif //DHT_API_H
