//
// Created by maxib on 8/24/2021.
//

#ifndef DHT_SECURERPCCONTEXT_H
#define DHT_SECURERPCCONTEXT_H

#include <kj/common.h>
#include <kj/refcount.h>
#include <kj/threadlocal.h>

namespace rpc
{
    class SecureRpcContext;

    inline KJ_THREADLOCAL_PTR(SecureRpcContext)threadSecureContext = nullptr;

    class SecureRpcContext : public kj::Refcounted
    {
    public:
        SecureRpcContext() : ioContext(kj::setupAsyncIo())
        {
            threadSecureContext = this;
        }

        ~SecureRpcContext() noexcept(false) override
        {
            KJ_REQUIRE(threadSecureContext == this,
                       "EzRpcContext destroyed from different thread than it was created.") { return; }
            threadSecureContext = nullptr;
        }

        kj::WaitScope &getWaitScope()
        {
            return ioContext.waitScope;
        }

        kj::AsyncIoProvider &getIoProvider()
        {
            return *ioContext.provider;
        }

        kj::LowLevelAsyncIoProvider &getLowLevelIoProvider()
        {
            return *ioContext.lowLevelProvider;
        }

        static kj::Own<SecureRpcContext> getThreadLocal()
        {
            SecureRpcContext *existing = threadSecureContext;
            if (existing != nullptr) {
                return kj::addRef(*existing);
            } else {
                return kj::refcounted<SecureRpcContext>();
            }
        }

    private:
        kj::AsyncIoContext ioContext{};
    };
}

#endif //DHT_SECURERPCCONTEXT_H
