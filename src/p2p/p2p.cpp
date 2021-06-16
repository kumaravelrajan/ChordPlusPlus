#include "p2p.h"

#include <iostream>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <kj/io.h>
#include <kj/async.h>
#include <person.capnp.h>

using p2p::P2p;
using capnp::MallocMessageBuilder;
using capnp::writePackedMessageToFd;

P2p::P2p(const Options &options)
{
    std::cout << "construct p2p" << std::endl;
}

P2p::~P2p()
{
    std::cout << "destruct p2p" << std::endl;
}

void p2p::testServer()
{
    capnp::EzRpcServer server{kj::heap<p2p::PeerImpl>(), "127.0.0.1", 6969};
    auto &waitScope = server.getWaitScope();

    // Run forever, accepting connections and handling requests.
    kj::NEVER_DONE.wait(waitScope);
}

void p2p::testClient()
{
    capnp::EzRpcClient client("127.0.0.1", 6969);
    auto &waitScope = client.getWaitScope();

    Peer::Client cap = client.getMain<Peer>();

    auto request = cap.getSuccessorRequest();
    request.setKey(
        capnp::Data::Builder(kj::heapArray<kj::byte>(std::initializer_list<kj::byte>{0x01, 0x23, 0x45, 0x67})));
    auto promise = request.send();

    auto response = promise.wait(waitScope);

    auto peerInfo = response.getPeerInfo();
    auto key = peerInfo.getKey();
    auto ip = peerInfo.getIp();
    std::cout
        << "Key:  " << std::string(key.asChars().begin(), key.asChars().end()) << '\n'
        << "IP:   " << std::string(ip.begin(), ip.end()) << '\n'
        << "Port: " << size_t(peerInfo.getPort()) << std::endl;
}