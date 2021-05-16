#include "connection.h"

Connection::Connection(asio::error_code error, tcp::socket &&sock):
    socket(std::move(sock))
{
    session = std::async(std::launch::async, [this, e{error}]() {
        using namespace std::chrono_literals;
        std::cout << "[API.connection] connected!" << std::endl;

        auto error{e};
        char data[1024];
        bool isRunning { true };
        for (; isRunning;) {
            size_t length = socket.read_some(asio::buffer(data), error);
            if (error == asio::error::eof) break;

            if (length > 0ull) {
                std::cout << "Got Data!" << std::endl;
                util::hexdump(util::convertToBytes(std::string(data, length)));
            } else {
                // std::cout << "asd" << std::endl;
            }
        }

        std::cout << "[API.connection] disconnected!" << std::endl;
    });
}

void Connection::close()
{
    socket.close();
}