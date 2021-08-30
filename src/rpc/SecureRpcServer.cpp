#include <kj/common.h>
#include <kj/string.h>
#include <kj/async-io.h>
#include <kj/debug.h>
#include <capnp/rpc-twoparty.h>
#include <map>
#include <utility>

#include "SecureRpcServer.h"

using rpc::SecureRpcServer;
using rpc::SecureRpcContext;

namespace
{
    class DummyFilter : public kj::LowLevelAsyncIoProvider::NetworkFilter
    {
    public:
        bool shouldAllow(const struct sockaddr *, kj::uint) override
        {
            return true;
        }
    };

    static DummyFilter DUMMY_FILTER;
}

struct SecureRpcServer::Impl final : public capnp::SturdyRefRestorer<capnp::AnyPointer>,
                                     public kj::TaskSet::ErrorHandler
{
    AsyncIoStreamFactory streamFactory;
    capnp::Capability::Client mainInterface;
    kj::Own<SecureRpcContext> context;

    struct ExportedCap
    {
        kj::String name;
        capnp::Capability::Client cap = nullptr;

        ExportedCap(kj::StringPtr name, capnp::Capability::Client cap)
            : name(kj::heapString(name)), cap(kj::mv(cap)) {}

        ExportedCap() = default;
        ExportedCap(const ExportedCap &) = delete;
        ExportedCap(ExportedCap &&) = default;
        ExportedCap &operator=(const ExportedCap &) = delete;
        ExportedCap &operator=(ExportedCap &&) = default;
    };

    std::map<kj::StringPtr, ExportedCap> exportMap;

    kj::ForkedPromise<kj::uint> portPromise;

    kj::TaskSet tasks;

    struct ServerContext
    {
        kj::Own<kj::AsyncIoStream> stream;
        capnp::TwoPartyVatNetwork network;
        capnp::RpcSystem<capnp::rpc::twoparty::VatId> rpcSystem;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        ServerContext(kj::Own<kj::AsyncIoStream> &&stream, SturdyRefRestorer <capnp::AnyPointer> &restorer,
                      capnp::ReaderOptions readerOpts)
            : stream(kj::mv(stream)),
              network(*this->stream, capnp::rpc::twoparty::Side::SERVER, readerOpts),
              rpcSystem(makeRpcServer(network, restorer)) {}
#pragma GCC diagnostic pop
    };

    Impl(capnp::Capability::Client mainInterface, kj::StringPtr bindAddress, uint defaultPort,
         capnp::ReaderOptions readerOpts,
         AsyncIoStreamFactory streamFactory)
        : streamFactory(kj::mv(streamFactory)),
          mainInterface(kj::mv(mainInterface)),
          context(SecureRpcContext::getThreadLocal()), portPromise(nullptr), tasks(*this)
    {
        auto paf = kj::newPromiseAndFulfiller<uint>();
        portPromise = paf.promise.fork();

        tasks.add(
            context->getIoProvider().getNetwork().parseAddress(bindAddress, defaultPort)
                .then(kj::mvCapture(
                    paf.fulfiller,
                    [this, readerOpts](kj::Own<kj::PromiseFulfiller<uint>> &&portFulfiller,
                                       kj::Own<kj::NetworkAddress> &&addr) {
                        auto listener = addr->listen();
                        portFulfiller->fulfill(listener->getPort());
                        acceptLoop(kj::mv(listener), readerOpts);
                    })));
    }

    void acceptLoop(kj::Own<kj::ConnectionReceiver> &&listener, capnp::ReaderOptions readerOpts)
    {
        auto ptr = listener.get();
        tasks.add(ptr->accept().then(kj::mvCapture(
            kj::mv(listener),
            [this, readerOpts](kj::Own<kj::ConnectionReceiver> &&listener,
                               kj::Own<kj::AsyncIoStream> &&connection) {
                acceptLoop(kj::mv(listener), readerOpts);

                return streamFactory(kj::mv(connection)).then([this, readerOpts](kj::Own<kj::AsyncIoStream> &&str) {
                    auto server = kj::heap<ServerContext>(kj::mv(str), *this, readerOpts);

                    // Arrange to destroy the server context when all references are gone, or when the
                    // EzRpcServer is destroyed (which will destroy the TaskSet).
                    tasks.add(server->network.onDisconnect().attach(kj::mv(server)));
                });
            }))
        );
    }

    capnp::Capability::Client restore(capnp::AnyPointer::Reader objectId) override
    {
        if (objectId.isNull()) {
            return mainInterface;
        } else {
            auto name = objectId.getAs<capnp::Text>();
            auto iter = exportMap.find(name);
            if (iter == exportMap.end()) {
                KJ_FAIL_REQUIRE("Server exports no such capability.", name) { break; }
                return nullptr;
            } else {
                return iter->second.cap;
            }
        }
    }

    void taskFailed(kj::Exception &&exception) override
    {
        kj::throwFatalException(kj::mv(exception));
    }
};

SecureRpcServer::SecureRpcServer(
    capnp::Capability::Client mainInterface, kj::StringPtr bindAddress,
    kj::uint defaultPort, capnp::ReaderOptions readerOpts,
    AsyncIoStreamFactory streamFactory)
    : impl(kj::heap<Impl>(kj::mv(mainInterface), bindAddress, defaultPort, readerOpts, kj::mv(streamFactory))) {}

SecureRpcServer::~SecureRpcServer() noexcept(false) {}

void SecureRpcServer::exportCap(kj::StringPtr name, capnp::Capability::Client cap)
{
    Impl::ExportedCap entry(kj::heapString(name), cap);
    impl->exportMap[entry.name] = kj::mv(entry);
}

kj::Promise<uint> SecureRpcServer::getPort()
{
    return impl->portPromise.addBranch();
}

kj::WaitScope &SecureRpcServer::getWaitScope()
{
    return impl->context->getWaitScope();
}

kj::AsyncIoProvider &SecureRpcServer::getIoProvider()
{
    return impl->context->getIoProvider();
}

kj::LowLevelAsyncIoProvider &SecureRpcServer::getLowLevelIoProvider()
{
    return impl->context->getLowLevelIoProvider();
}