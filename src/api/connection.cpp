#include "api.h"
#include <cstdint>

API::Connection::Connection(asio::error_code error, tcp::socket &&sock, const Api &api):
    socket(std::move(sock)),
    api(api)
{
    start_read();
    /*
    session = std::async(std::launch::async, [this, e { error }, &api]() {
        using namespace std::chrono_literals;
        std::cout << "[API.connection] connected!" << std::endl;

        auto error { e };
        std::vector<uint8_t> data((1 << 16) - 1, 0);
        while (socket.is_open()) {
            socket.async_read_some(asio::buffer(data), [](const asio::error_code &e, std::size_t length) {});
            size_t length = socket.read_some(asio::buffer(data), error);
            if (error == asio::error::eof) break;

            if (length > 0ull) {
                std::cout << "Got Data!" << std::endl;
                auto bytes = util::convertToBytes(data, length);
                util::hexdump(bytes);

                Request *request;
                try {
                    request = new Request(bytes);
                }
                catch (const std::exception &e) {
                    std::cerr << "[API.connection] Exception thrown when parsing request:\n";
                    std::cerr << e.what() << std::endl;
                    continue;
                }
                if (socket.is_open() && api.requestHandler) {
                    std::vector<std::byte> response = api.requestHandler.value()(*request);
                    if (!response.empty()) {
                        socket.write_some(asio::buffer(response));
                    }
                }
            }
        }

        std::cout << "[API.connection] disconnected!" << std::endl;
        done = true;
    });
     */
}

void API::Connection::start_read()
{
    if (!socket.is_open()) {
        std::cout << "[API.connection] disconnected!" << std::endl;
        done = true;
        return;
    }
    socket.async_read_some(asio::buffer(data), [this](const asio::error_code &error, std::size_t length) {
        start_read();

        if (error) {
            std::cerr << "[API.connection] " << error.message() << std::endl;
            return;
        }

        if (length > 0ull) {
            std::cout << "Got Data!" << std::endl;
            auto bytes = util::convertToBytes(data, length);
            util::hexdump(bytes);

            Request *request;
            try {
                request = new Request(bytes);
            }
            catch (const std::exception &e) {
                std::cerr << "[API.connection] Exception thrown when parsing request:\n";
                std::cerr << e.what() << std::endl;
                return;
            }
            if (socket.is_open() && api.requestHandler) {
                std::vector<std::byte> response = api.requestHandler.value()(*request);
                if (!response.empty()) {
                    socket.write_some(asio::buffer(response));
                }
            }
        }
    });
}

void API::Connection::close()
{
    socket.close();
}

bool API::Connection::isDone()
{
    return done;
}