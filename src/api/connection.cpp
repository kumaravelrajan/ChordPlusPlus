#include "api.h"
#include <cstdint>

API::Connection::Connection(tcp::socket &&sock, const Api &api):
    socket(std::move(sock)),
    api(api)
{
    start_read();
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
            if (socket.is_open()) {
                auto t = request->getData()->m_header.msg_type;

                if (api.requestHandlers.contains(t)) {
                    std::vector<std::byte> response = api.requestHandlers.find(t)->second(*request->getData<>());
                    if (!response.empty()) {
                        socket.write_some(asio::buffer(response));
                    }
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