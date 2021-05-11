#ifndef DHT_API_H
#define DHT_API_H

#include <future>
#include <memory>
#include <queue>
#include <optional>
#include <set>
#include "request.h"

class Api
{
public:
    Api();
    ~Api();

    const std::queue<Request> newRequests;

private:
    void run();

    bool isRunning;
    std::future<void> server;

};

#endif //DHT_API_H
