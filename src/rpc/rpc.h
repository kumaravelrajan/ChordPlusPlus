#ifndef DHT_RPC_H
#define DHT_RPC_H

#include "SecureRpcClient.h"
#include "SecureRpcServer.h"
#include <config.h>

namespace rpc
{
    kj::Own<SecureRpcClient>
    getClient(const config::Configuration &conf, const std::string &ip, uint16_t defaultPort = 0);

    kj::Own<rpc::SecureRpcServer>
    getServer(const config::Configuration &conf, capnp::Capability::Client mainInterface,
              kj::StringPtr bindAddress, kj::uint defaultPort = 0);
}

#endif //DHT_RPC_H
