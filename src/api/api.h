#ifndef DHT_API_H
#define DHT_API_H

#include <future>
#include <memory>
#include <queue>
#include <optional>
#include <set>
#include <asio.hpp>
#include "request.h"

namespace API
{
    class Api
    {
    public:
        struct Options
        {
        };

        explicit Api(const Options &o = {});
        ~Api();

        const std::queue<Request> newRequests;

    private:
        void run();

        bool isRunning;
        std::future<void> server;
    };
} // namespace API

#endif //DHT_API_H
