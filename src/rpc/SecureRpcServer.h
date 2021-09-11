#ifndef DHT_SECURE_RPC_SERVER_H
#define DHT_SECURE_RPC_SERVER_H

#include <capnp/rpc.h>
#include <capnp/message.h>
#include <kj/async-io.h>

#include "SecureRpcContext.h"

namespace rpc
{
    /**
     * @brief The server counterpart to `SecureRpcClient`.
     */
    class SecureRpcServer
    {
    public:
        /**
         * @brief
         * Construct a new `SecureRpcServer` that binds to the given address.  An address of "*" means to
         * bind to all local addresses.
         * <br/><br/>
         * `defaultPort` is the IP port number to use if `serverAddress` does not include it explicitly.
         * If unspecified, a port is chosen automatically, and you must call getPort() to find out what
         * it is.
         * <br/><br/>
         * The address is parsed by `kj::Network` in `kj/async-io.h`.  See that interface for more info
         * on the address format, but basically it's what you'd expect.
         * <br/><br/>
         * The server might not begin listening immediately, especially if `bindAddress` needs to be
         * resolved.  If you need to wait until the server is definitely up, wait on the promise returned
         * by `getPort()`.
         * <br/><br/>
         * `readerOpts` is the ReaderOptions structure used to read each incoming message on the
         * connection. Setting this may be necessary if you need to receive very large individual
         * messages or messages. However, it is recommended that you instead think about how to change
         * your protocol to send large data blobs in multiple small chunks -- this is much better for
         * both security and performance. See `ReaderOptions` in `message.h` for more details.
         */
        explicit SecureRpcServer(capnp::Capability::Client mainInterface, kj::StringPtr bindAddress,
                                 kj::uint defaultPort = 0, capnp::ReaderOptions readerOpts = capnp::ReaderOptions(),
                                 AsyncIoStreamFactory streamFactory =
                                 [](kj::Own<kj::AsyncIoStream> str) { return kj::mv(str); });

        ~SecureRpcServer() noexcept(false);

        /**
         * @brief
         * Export a capability publicly under the given name, so that clients can import it.
         * Keep in mind that you can implicitly convert `kj::Own<MyType::Server>&&` to
         * `Capability::Client`, so it's typical to pass something like
         * `kj::heap<MyImplementation>(<constructor params>)` as the second parameter.
         */
        void exportCap(kj::StringPtr name, capnp::Capability::Client cap);

        /**
         * @brief
         * Get the IP port number on which this server is listening.  This promise won't resolve until
         * the server is actually listening.  If the address was not an IP address (e.g. it was a Unix
         * domain socket) then getPort() resolves to zero.
         */
        kj::Promise<kj::uint> getPort();

        /**
         * @brief
         * Get the `WaitScope` for the client's `EventLoop`, which allows you to synchronously wait on
         * promises.
         */
        kj::WaitScope &getWaitScope();

        /**
         * @brief
         * Get the underlying AsyncIoProvider set up by the RPC system.  This is useful if you want
         * to do some non-RPC I/O in asynchronous fashion.
         * @return
         */
        kj::AsyncIoProvider &getIoProvider();

        /**
         * @brief
         * Get the underlying LowLevelAsyncIoProvider set up by the RPC system.  This is useful if you
         * want to do some non-RPC I/O in asynchronous fashion.
         */
        kj::LowLevelAsyncIoProvider &getLowLevelIoProvider();

    private:
        struct Impl;
        kj::Own<Impl> impl;
    };
}

#endif //DHT_SECURE_RPC_SERVER_H
