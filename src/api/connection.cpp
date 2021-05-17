#include "api.h"
#include <cstdint>

API::Connection::Connection(asio::error_code error, tcp::socket &&sock, const Api &api):
    socket(std::move(sock)),
    api(api)
{
    session = std::async(std::launch::async, [this, e { error }, &api]() {
        using namespace std::chrono_literals;
        std::cout << "[API.connection] connected!" << std::endl;

        auto error { e };
        std::array<uint8_t, (1 << 16) - 1> data { 0 };
        for (;;) {
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
                if (api.requestHandler) {
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
}

void API::Connection::close()
{
    socket.close();
}

void API::Connection::get()
{
    session.wait();
}

bool API::Connection::isDone()
{
    return done;
}