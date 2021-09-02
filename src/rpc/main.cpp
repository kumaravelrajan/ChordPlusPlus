#include <iostream>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/bundled/color.h>
#include <future>
#include <capnp/ez-rpc.h>
#include <atomic>
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

int main()
{
    std::atomic_bool started{false};
    std::atomic_bool done{false};

    auto fs = kj::newDiskFilesystem();

    auto client = std::async(std::launch::async, [&done, &started] {
        while (!started);

        kj::TlsContext::Options options{};
        options.ignoreCertificates = true;
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
        try {
            auto result = req.send().then([](Example::AddResults::Reader &&reader) {
                return reader;
            }, [](kj::Exception &&err) -> Example::AddResults::Reader {
                std::cout << fmt::format("send Exception: {}", err.getDescription().cStr()) << std::endl;
                throw std::runtime_error("nah dude");
            }).wait(client.getWaitScope());
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
        } catch (kj::Exception &err) {
            std::cout << fmt::format("Error: {}", err.getDescription().cStr()) << std::endl;
        }
        done = true;
    });

    auto server = std::async(std::launch::async, [&done, &started, &fs] {
        kj::TlsContext::Options options{};
        auto key = fs->getRoot().openFile(fs->getCurrentPath().eval("../../../hostkey.pem"))->readAllText();
        auto cert = fs->getRoot().openFile(fs->getCurrentPath().eval("../../../hostcert.crt"))->readAllText();
        kj::TlsKeypair kp{
            kj::TlsPrivateKey(key, kj::StringPtr("asdf")),
            kj::TlsCertificate(cert)
        };
        options.defaultKeypair = kp;
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
