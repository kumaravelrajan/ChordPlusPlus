#include "rpc.h"
#include <kj/compat/tls.h>
#include <kj/filesystem.h>

kj::Own<rpc::SecureRpcClient>
rpc::getClient(const config::Configuration &conf, const std::string &ip, uint16_t defaultPort)
{
    auto fs = kj::newDiskFilesystem();
    kj::TlsContext::Options options{};
    options.ignoreCertificates = true;
    auto key = fs->getRoot().openFile(fs->getCurrentPath().eval(conf.private_key_path))->readAllText();
    auto cert = fs->getRoot().openFile(fs->getCurrentPath().eval(conf.certificate_path))->readAllText();
    kj::TlsKeypair kp{
        kj::TlsPrivateKey(key, conf.private_key_password
                               ? kj::Maybe<kj::StringPtr>(*conf.private_key_password)
                               : kj::Maybe<kj::StringPtr>()),
        kj::TlsCertificate(cert)
    };
    auto tlsContext = kj::heap<kj::TlsContext>(options);

    return kj::heap<rpc::SecureRpcClient>(
        ip, defaultPort, capnp::ReaderOptions(),
        [tlsContext(&*tlsContext)](kj::Own<kj::AsyncIoStream> &&str) {
            return tlsContext->wrapClient(kj::mv(str), "hostname")
                .then([](kj::Own<kj::AsyncIoStream> &&str) {
                    return kj::mv(str);
                });
        }
    ).attach(kj::mv(tlsContext));
}

kj::Own<rpc::SecureRpcServer>
rpc::getServer(const config::Configuration &conf, capnp::Capability::Client mainInterface,
               kj::StringPtr bindAddress, kj::uint defaultPort)
{
    auto fs = kj::newDiskFilesystem();
    kj::TlsContext::Options options{};
    auto key = fs->getRoot().openFile(fs->getCurrentPath().eval(conf.private_key_path))->readAllText();
    auto cert = fs->getRoot().openFile(fs->getCurrentPath().eval(conf.certificate_path))->readAllText();
    kj::TlsKeypair kp{
        kj::TlsPrivateKey(key, conf.private_key_password
                               ? kj::Maybe<kj::StringPtr>(*conf.private_key_password)
                               : kj::Maybe<kj::StringPtr>()),
        kj::TlsCertificate(cert)
    };
    options.defaultKeypair = kp;
    auto tlsContext = kj::heap<kj::TlsContext>(options);

    return kj::heap<rpc::SecureRpcServer>(
        kj::mv(mainInterface), bindAddress, defaultPort, capnp::ReaderOptions(),
        [tlsContext(&*tlsContext)](kj::Own<kj::AsyncIoStream> &&str) {
            return tlsContext->wrapServer(kj::mv(str)).then([](kj::Own<kj::AsyncIoStream> &&str) {
                return kj::mv(str);
            });
        }
    ).attach(kj::mv(tlsContext));
}
