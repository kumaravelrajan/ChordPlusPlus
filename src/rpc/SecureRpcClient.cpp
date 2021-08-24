#include <kj/common.h>
#include <kj/string.h>
#include <kj/async-io.h>
#include <capnp/rpc-twoparty.h>

#include "SecureRpcClient.h"
#include "SecureRpcContext.h"

using rpc::SecureRpcClient;
using rpc::SecureRpcContext;

kj::Promise<kj::Own<kj::AsyncIoStream>> connectAttach(kj::Own<kj::NetworkAddress> &&addr)
{
    return addr->connect().attach(kj::mv(addr));
}

struct SecureRpcClient::Impl
{
    kj::Own<SecureRpcContext> context;

    struct ClientContext
    {
        kj::Own<kj::AsyncIoStream> stream;
        capnp::TwoPartyVatNetwork network;
        capnp::RpcSystem<capnp::rpc::twoparty::VatId> rpcSystem;

        ClientContext(kj::Own<kj::AsyncIoStream> &&stream, capnp::ReaderOptions readerOpts)
            : stream(kj::mv(stream)),
              network(*this->stream, capnp::rpc::twoparty::Side::CLIENT, readerOpts),
              rpcSystem(makeRpcClient(network)) {}

        capnp::Capability::Client getMain()
        {
            capnp::word scratch[4];
            memset(scratch, 0, sizeof(scratch));
            capnp::MallocMessageBuilder message(scratch);
            auto hostId = message.getRoot<capnp::rpc::twoparty::VatId>();
            hostId.setSide(capnp::rpc::twoparty::Side::SERVER);
            return rpcSystem.bootstrap(hostId);
        }

        capnp::Capability::Client restore(kj::StringPtr name)
        {
            capnp::word scratch[64];
            memset(scratch, 0, sizeof(scratch));
            capnp::MallocMessageBuilder message(scratch);

            auto hostIdOrphan = message.getOrphanage().newOrphan<capnp::rpc::twoparty::VatId>();
            auto hostId = hostIdOrphan.get();
            hostId.setSide(capnp::rpc::twoparty::Side::SERVER);

            auto objectId = message.getRoot<capnp::AnyPointer>();
            objectId.setAs<capnp::Text>(name);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            return rpcSystem.restore(hostId, objectId);
#pragma GCC diagnostic pop
        }
    };

    kj::ForkedPromise<void> setupPromise;

    kj::Maybe<kj::Own<ClientContext>> clientContext;

    Impl(kj::StringPtr serverAddress, kj::uint defaultPort,
         capnp::ReaderOptions readerOpts)
        : context(SecureRpcContext::getThreadLocal()),
          setupPromise(
              context->getIoProvider().getNetwork()
                  .parseAddress(serverAddress, defaultPort)
                  .then([](kj::Own<kj::NetworkAddress> &&addr) {
                      return connectAttach(kj::mv(addr));
                  }).then([this, readerOpts](kj::Own<kj::AsyncIoStream> &&stream) {
                      clientContext = kj::heap<ClientContext>(kj::mv(stream),
                                                              readerOpts);
                  }).fork()) {}
};

SecureRpcClient::SecureRpcClient(
    kj::StringPtr serverAddress, kj::uint defaultPort, capnp::ReaderOptions readerOpts)
    : impl(kj::heap<Impl>(serverAddress, defaultPort, readerOpts)) {}

SecureRpcClient::~SecureRpcClient() noexcept(false) {}

capnp::Capability::Client SecureRpcClient::getMain()
{
    KJ_IF_MAYBE(client, impl->clientContext) {
        return client->get()->getMain();
    } else {
        return impl->setupPromise.addBranch().then([this]() {
            return KJ_ASSERT_NONNULL(impl->clientContext)->getMain();
        });
    }
}

kj::WaitScope &SecureRpcClient::getWaitScope()
{
    return impl->context->getWaitScope();
}

kj::AsyncIoProvider &SecureRpcClient::getIoProvider()
{
    return impl->context->getIoProvider();
}

kj::LowLevelAsyncIoProvider &SecureRpcClient::getLowLevelIoProvider()
{
    return impl->context->getLowLevelIoProvider();
}