#ifndef DHT_API_H
#define DHT_API_H

#include <future>
#include <memory>
#include <queue>
#include <optional>
#include <set>
#include <asio.hpp>
#include "request.h"
#include "connection.h"

namespace API
{
    class Api
    {
        using tcp = asio::ip::tcp;

    public:
        struct Options
        {
        };

        explicit Api(const Options &o = {});
        ~Api();

        const std::queue<Request> newRequests;

    private:
        void start_accept();

        std::unique_ptr<asio::io_service> service;
        std::unique_ptr<tcp::acceptor> acceptor;
        std::future<void> service_future;

        std::vector<std::unique_ptr<Connection>> openConnections;

        bool isRunning = false;
    };
} // namespace API

#endif //DHT_API_H
