#include <iostream>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/bundled/color.h>
#include <future>
#include <capnp/ez-rpc.h>
#include <kj/common.h>
#include <atomic>
#include <chrono>
#include <example.capnp.h>
#include <kj/compat/tls.h>
#include <kj/filesystem.h>

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

    static kj::Promise<kj::Array<kj::byte>> convert(const kj::ArrayPtr<const kj::byte> &arr)
    {
        auto ret = kj::heapArray<kj::byte>(arr);
        for (auto &num : ret) {
            if (num == 123) num = 124;
        }
        return kj::mv(ret);
    }

    kj::Promise<void> write(const void *buffer, size_t size) override
    {
        auto buf = kj::heapArray<kj::byte>((kj::byte *) buffer, size);
        auto ptr = kj::heapArray<const kj::ArrayPtr<const kj::byte>>({buf});
        return write(ptr).attach(kj::mv(buf), kj::mv(ptr));
    }

    kj::Promise<void> write(kj::ArrayPtr<const kj::ArrayPtr<const kj::byte>> pieces) override
    {
        auto results = kj::heapArrayBuilder<kj::Promise<kj::Array<kj::byte>>>(pieces.size());
        for (auto &piece : pieces)
            results.add(convert(piece));
        return kj::joinPromises(results.finish()).then([this](kj::Array<kj::Array<kj::byte>> &&arr) {
            auto pointers = kj::heapArray<const kj::ArrayPtr<const kj::byte>>(arr.begin(), arr.end());
            return impl->write(pointers.asPtr()).attach(kj::mv(arr), kj::mv(pointers));
        }).attach(kj::mv(results));
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

    auto fs = kj::newDiskFilesystem();

    auto client = std::async(std::launch::async, [&done, &started, &fs] {
        while (!started);

        kj::TlsContext::Options options{};
        auto trusted = kj::heapArray<kj::TlsCertificate>(
            {
                kj::TlsCertificate{
                    fs->getRoot().openFile(fs->getCurrentPath().eval("../../../hostcert.crt"))->readAllText()
                }
            }
        );
        // options.verifyClients = true;
        options.useSystemTrustStore = true;
        options.trustedCertificates = trusted;
        kj::TlsContext tlsContext{options};

        rpc::SecureRpcClient client(
            "127.0.0.1", 1234, capnp::ReaderOptions(),
            [&tlsContext](kj::Own<kj::AsyncIoStream> &&str) {
                return tlsContext
                    .wrapClient(kj::mv(str), "COLGATE")
                    .then([](kj::Own<kj::AsyncIoStream> &&str) {
                        std::cout << "wrapClient done" << std::endl;
                        return kj::mv(str);
                    });
            }
        );
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

    auto server = std::async(std::launch::async, [&done, &started, &fs] {
        class CB final : public kj::TlsSniCallback
        {
        public:
            explicit CB(kj::Filesystem &fs) : fs(fs) {}
        private:
            kj::Filesystem &fs;

            kj::Maybe<kj::TlsKeypair> getKey(kj::StringPtr hostname) override
            {
                if (hostname == "COLGATE") {
                    auto key = fs.getRoot().openFile(fs.getCurrentPath().eval("../../../hostkey.pem"))->readAllText();
                    auto cert = fs.getRoot().openFile(fs.getCurrentPath().eval("../../../hostcert.crt"))->readAllText();

                    return kj::TlsKeypair{
                        kj::TlsPrivateKey(key, kj::StringPtr("asdf")),
                        kj::TlsCertificate(cert)
                    };
                }
                return nullptr;
            }
        };
        CB cb(*fs);
        kj::TlsContext::Options options{};
        options.sniCallback = cb;
        kj::TlsContext tlsContext{options};

        rpc::SecureRpcServer server(
            kj::heap<ExampleImpl>(), "127.0.0.1", 1234, capnp::ReaderOptions(),
            [&tlsContext](kj::Own<kj::AsyncIoStream> &&str) {
                return tlsContext.wrapServer(kj::mv(str)).then([](kj::Own<kj::AsyncIoStream> &&str) {
                    std::cout << "wrapServer done" << std::endl;
                    return kj::mv(str);
                });
            }
        );
        while (!done) {
            server.getWaitScope().poll();
            if (!started) started = true;
        }
    });

    client.wait();
    server.wait();

    return 0;
}
