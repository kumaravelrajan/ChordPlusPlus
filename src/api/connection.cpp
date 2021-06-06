#include "api.h"
#include <cstdint>

api::Connection::Connection(tcp::socket &&sock, const Api &api):
    socket(std::move(sock)),
    api(api)
{
    start_read();

    handlerCallManager = std::async(std::launch::async, [this]() {
        using namespace std::chrono_literals;

        while (socket.is_open()) {

            if (!handlerCalls.empty() && handlerCalls[0].wait_for(0s) == std::future_status::ready) {

                handlerCallMutex.lock();
                auto bytes = handlerCalls.front().get();
                handlerCalls.pop_front();
                handlerCallMutex.unlock();

                if (socket.is_open() && !bytes.empty()) {
                    socket.write_some(asio::buffer(bytes));
                }
            }

            std::this_thread::sleep_for(100ms);
        }
        done = true;
    });
}

void api::Connection::start_read()
{
    if (!socket.is_open()) {
        std::cout << "[API.connection] disconnected!" << std::endl;
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

            std::unique_ptr<Request> request;
            try {
                request = std::make_unique<Request>(bytes);
            }
            catch (const std::exception &e) {
                std::cerr << "[API.connection] Exception thrown when parsing request:\n";
                std::cerr << e.what() << std::endl;
                return;
            }
            if (socket.is_open()) {
                auto t = request->getData()->m_header.msg_type;
                if (api.requestHandlers.contains(t)) {
                    handlerCallMutex.lock();
                    handlerCalls.push_back(std::async(std::launch::async, [this, request(std::move(request))]() {
                        return api.requestHandlers.find(request->getData()->m_header.msg_type)->second(*(request->getData<>()), cancellation_token);
                    }));
                    handlerCallMutex.unlock();
                }
            }
        }
    });
}

void api::Connection::close()
{
    socket.close();
}

bool api::Connection::isDone() const
{
    return done;
}

api::Connection::~Connection()
{
    cancellation_token = true;
}