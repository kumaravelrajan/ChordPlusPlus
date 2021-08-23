#include <iostream>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/bundled/color.h>
#include <future>
#include <capnp/ez-rpc.h>
#include <kj/common.h>
#include <atomic>
#include <chrono>
#include <example.capnp.h>

#include "rpc.h"

using namespace std::literals;

class ExampleImpl : public Example::Server
{
    kj::Promise<void> add(AddContext context) override
    {
        auto res = context.getResults();
        res.setSum(context.getParams().getA() + context.getParams().getB());
        return kj::READY_NOW;
    }
};

int main()
{
    std::atomic_bool started{false};
    std::atomic_bool done{false};

    auto client = std::async(std::launch::async, [&done, &started] {
        while (!started);
        capnp::EzRpcClient client("127.0.0.1", 1234);
        auto cap = client.getMain<Example>();
        auto req = cap.addRequest();
        int a = 123;
        int b = 297;
        req.setA(a);
        req.setB(b);
        auto result = req.send().wait(client.getWaitScope()).getSum();
        std::cout << fmt::format(
            fmt::emphasis::bold,
            "{} {} {} {} {}",
            fmt::format(fg(fmt::color::aquamarine), "{}", a),
            fmt::format(fg(fmt::color::dark_cyan), "+"),
            fmt::format(fg(fmt::color::aquamarine), "{}", b),
            fmt::format(fg(fmt::color::dark_cyan), "="),
            fmt::format(fg(fmt::color::aqua), "{}", result)
        ) << std::endl;
        done = true;
    });

    auto server = std::async(std::launch::async, [&done, &started] {
        capnp::EzRpcServer server(kj::heap<ExampleImpl>(), "127.0.0.1", 1234);
        while (!done) {
            server.getWaitScope().poll();
            if (!started) started = true;
        }
    });

    client.wait();
    server.wait();

    return 0;
}
