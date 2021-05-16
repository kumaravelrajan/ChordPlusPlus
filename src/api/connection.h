#ifndef DHT_CONNECTION_H
#define DHT_CONNECTION_H

#include <iostream>
#include <optional>
#include <future>
#include <asio.hpp>
#include <util.h>

class Connection
{
    using tcp = asio::ip::tcp;

public:
    Connection() = delete;
    Connection(asio::error_code error, tcp::socket &&socket);
    Connection(const Connection &other) = delete;
    Connection(Connection &&other) = delete;

    Connection &operator=(const Connection &other) = delete;
    Connection &operator=(Connection &&other) = delete;

    void close();

private:
    tcp::socket socket;
    std::future<void> session;
};

#endif //DHT_CONNECTION_H
