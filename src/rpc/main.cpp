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
        res.setA(context.getParams().getA());
        res.setB(context.getParams().getB());
        res.setSum(context.getParams().getA() + context.getParams().getB());
        return kj::READY_NOW;
    }
};

class ExampleAsyncIoStream final : public kj::AsyncIoStream
{
public:
    kj::Own<kj::AsyncIoStream> impl;
    explicit ExampleAsyncIoStream(kj::Own<kj::AsyncIoStream> impl)
        : impl(kj::mv(impl)) {}

    void shutdownWrite() override
    {
        impl->shutdownWrite();
    }

    void abortRead() override
    {
        impl->abortRead();
    }

    void getsockopt(int level, int option, void *value, uint *length) override
    {
        impl->getsockopt(level, option, value, length);
    }

    void setsockopt(int level, int option, const void *value, uint length) override
    {
        impl->setsockopt(level, option, value, length);
    }

    void getsockname(struct sockaddr *addr, uint *length) override
    {
        impl->getsockname(addr, length);
    }

    void getpeername(struct sockaddr *addr, uint *length) override
    {
        impl->getpeername(addr, length);
    }

    // Input

    kj::Promise<size_t> read(void *buffer, size_t minBytes, size_t maxBytes) override
    {
        return impl->read(buffer, minBytes, maxBytes);
    }

    kj::Promise<size_t> tryRead(void *buffer, size_t minBytes, size_t maxBytes) override
    {
        return impl->tryRead(buffer, minBytes, maxBytes);
    }

    kj::Maybe<uint64_t> tryGetLength() override
    {
        return impl->tryGetLength();
    }

    kj::Promise<uint64_t> pumpTo(
        AsyncOutputStream &output, uint64_t amount) override
    {
        return impl->pumpTo(output, amount);
    }

    // Output

    kj::Promise<void> write(const void *buffer, size_t size) override
    {
        auto b = std::make_unique<uint8_t[]>(size);
        memcpy(b.get(), buffer, size);
        for (size_t index = 0; index < size; ++index)
            if (b[index] == 123) b[index] = 124;
        return impl->write(b.get(), size).attach(std::move(b));
    }

    kj::Promise<void> write(kj::ArrayPtr<const kj::ArrayPtr<const kj::byte>> pieces) override
    {
        auto arr = kj::heapArray<kj::Array<kj::byte>>(pieces.size());
        for (size_t index = 0; index < pieces.size(); ++index)
            arr[index] = kj::heapArray(pieces[index]);
        auto ptrs = kj::heapArray<const kj::ArrayPtr<const kj::byte>>(arr.begin(), arr.end());

        for (auto &piece : arr)
            for (auto &num : piece)
                if (num == 123) num = 124;

        return impl->write(ptrs.asPtr()).attach(kj::mv(arr), kj::mv(ptrs));
    }

    kj::Maybe<kj::Promise<uint64_t>> tryPumpFrom(
        AsyncInputStream &input, uint64_t amount) override
    {
        return impl->tryPumpFrom(input, amount);
    }

    kj::Promise<void> whenWriteDisconnected() override
    {
        return impl->whenWriteDisconnected();
    }
};

int main()
{
    std::atomic_bool started{false};
    std::atomic_bool done{false};

    auto client = std::async(std::launch::async, [&done, &started] {
        while (!started);
        rpc::SecureRpcClient client(
            "127.0.0.1", 1234, capnp::ReaderOptions(),
            [](kj::Own<kj::AsyncIoStream> str) {
                return kj::heap<ExampleAsyncIoStream>(kj::mv(str));
            });
        auto cap = client.getMain<Example>();
        auto req = cap.addRequest();
        req.setA(123);
        req.setB(297);
        auto result = req.send().wait(client.getWaitScope());
        int a = result.getA();
        int b = result.getB();
        int sum = result.getSum();
        std::cout << fmt::format(
            fmt::emphasis::bold,
            "{} {} {} {} {}",
            fmt::format(fg(fmt::color::aquamarine), "{}", a),
            fmt::format(fg(fmt::color::dark_cyan), "+"),
            fmt::format(fg(fmt::color::aquamarine), "{}", b),
            fmt::format(fg(fmt::color::dark_cyan), "="),
            fmt::format(fg(fmt::color::aqua), "{}", sum)
        ) << std::endl;
        done = true;
    });

    auto server = std::async(std::launch::async, [&done, &started] {
        rpc::SecureRpcServer server(kj::heap<ExampleImpl>(), "127.0.0.1", 1234);
        while (!done) {
            server.getWaitScope().poll();
            if (!started) started = true;
        }
    });

    client.wait();
    server.wait();

    return 0;
}
