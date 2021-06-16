#ifndef DHT_P2P_H
#define DHT_P2P_H

#include <iostream>
#include <cstdint>
#include <vector>
#include <cstddef>
#include <optional>
#include <functional>
#include <string>
#include <capnp/capability.h>
#include <capnp/ez-rpc.h>
#include "peer.h"

namespace p2p
{
    struct Options
    {
        uint16_t port = 1234ull;
    };

    class P2p
    {


    public:
        explicit P2p(const Options &options);
        ~P2p();
    };

    void testServer();

    void testClient();
}

#endif //DHT_P2P_H
