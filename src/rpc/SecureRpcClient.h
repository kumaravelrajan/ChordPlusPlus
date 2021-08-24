#ifndef DHT_SECURE_RPC_CLIENT_H
#define DHT_SECURE_RPC_CLIENT_H

#include <capnp/ez-rpc.h>
#include <capnp/message.h>
#include <capnp/capability.h>
#include <kj/common.h>

extern capnp::EzRpcClient a;

namespace rpc
{
    /**
     * @brief
     * Secure interface for setting up a Cap'n Proto RPC client.
     * TODO:
     *   - Implement alternative networking solution with encryption
     */
    class SecureRpcClient
    {
    public:

        /**
         * @brief
         * Construct a new SecureRpcClient and connect to the given address. The connection is formed in
         * the background -- if it fails, calls to capabilities returned by importCap() will fail with an
         * appropriate exception.
         * <br/><br/>
         * `defaultPort` is the IP port number to use if `serverAddress` does not include it explicitly.
         * If unspecified, the port is required in `serverAddress`.
         * <br/><br/>
         * The address is parsed by `kj::Network` in `kj/async-io.h`. See that interface for more info
         * on the address format, but basically it's what you'd expect.
         * <br/><br/>
         * `readerOpts` is the ReaderOptions structure used to read each incoming message on the
         * connection. Setting this may be necessary if you need to receive very large individual
         * messages or messages. However, it is recommended that you instead think about how to change
         * your protocol to send large data blobs in multiple small chunks -- this is much better for
         * both security and performance. See `ReaderOptions` in `message.h` for more details.
         */
        explicit SecureRpcClient(kj::StringPtr serverAddress, kj::uint defaultPort = 0,
                                 capnp::ReaderOptions readerOpts = capnp::ReaderOptions());

        ~SecureRpcClient() noexcept(false);

        /**
         * @brief Get the server's main (aka "bootstrap") interface.
         * @tparam Type : The interface that contains a ::Client class.
         */
        template<typename Type>
        typename Type::Client getMain();
        capnp::Capability::Client getMain();

        /**
         * @brief
         * Get the `WaitScope` for the client's `EventLoop`, which allows you to synchronously wait on
         * promises.
         */
        kj::WaitScope &getWaitScope();

        /**
         * @brief
         * Get the underlying AsyncIoProvider set up by the RPC system. This is useful if you want
         * to do some non-RPC I/O in asynchronous fashion.
         */
        kj::AsyncIoProvider &getIoProvider();

        /**
         * Get the underlying LowLevelAsyncIoProvider set up by the RPC system.  This is useful if you
         * want to do some non-RPC I/O in asynchronous fashion.
         */
        kj::LowLevelAsyncIoProvider &getLowLevelIoProvider();

    private:
        struct Impl;
        kj::Own <Impl> impl;
    };

    // ================================================= //
    // ========[ inline implementation details ]======== //
    // ================================================= //

    template<typename Type>
    inline typename Type::Client SecureRpcClient::getMain()
    {
        return getMain().castAs<Type>();
    }
}

#endif //DHT_SECURE_RPC_CLIENT_H
