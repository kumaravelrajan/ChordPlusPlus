#include "Dht.h"
#include <util.h>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/ez-rpc.h>
#include "config/centralLogControl.h"

using dht::Dht;
using namespace std::chrono_literals;

void Dht::runServer()
{
    auto peerImpl = kj::heap<PeerImpl>(m_nodeInformation);
    m_peerImpl = *peerImpl;
    ::capnp::EzRpcServer peerServer{
        std::move(peerImpl),
        m_nodeInformation->getIp(),
        m_nodeInformation->getPort()
    };
    auto &waitScope = peerServer.getWaitScope();

    m_executor.emplace(kj::getCurrentThreadExecutor());

    // It seems like promises are atomic in the event loop. So no *actual* asynchronous execution... Threads it is then
    auto f = std::async(std::launch::async, [this]() {
        mainLoop();
    });

    while (m_dhtRunning) {
        waitScope.poll();
    }

    m_peerImpl.reset();
}

void Dht::mainLoop()
{
    /*
     * This is where the dht will fix fingers, store and retrieve data, etc.
     * This needs to be thread-safe, and needs to be able to exit at any time.
     * (No blocking function calls in here, at least not for too long)
     */

    SPDLOG_INFO("[DHT] Main Loop Entered");

    std::this_thread::sleep_for(5s);

    while (true) {
        if (!m_dhtRunning) break;
        if (!m_nodeInformation->getSuccessor()) {
            if (m_nodeInformation->getBootstrapNode() &&
                m_nodeInformation->getBootstrapNode() != m_nodeInformation->getNode()) {
                join(*m_nodeInformation->getBootstrapNode());
            } else {
                create();
            }
        }

        if (!m_dhtRunning) break;
        stabilize();
        if (!m_dhtRunning) break;
        fixFingers();
        if (!m_dhtRunning) break;
        checkPredecessor();
        if (!m_dhtRunning) break;

        // Wait one second
        std::this_thread::sleep_for(1s);
        // std::cout << "Main loop, m_dhtRunning: " << m_dhtRunning << std::endl;
    }

    SPDLOG_INFO("[DHT] Exiting Main Loop");
}

void Dht::setApi(std::unique_ptr<api::Api> api)
{
    // Destroy old api (This is in two statements for easier debugging):
    m_api = nullptr;
    // Move in new api:
    m_api = std::move(api);

    // Set Request handlers:
    m_api->on<util::constants::DHT_PUT>(
        [this](const api::Message_DHT_PUT &m, std::atomic_bool &cancelled) {
            return onDhtPut(m, cancelled);
        });
    m_api->on<util::constants::DHT_GET>(
        [this](const api::Message_KEY &m, std::atomic_bool &cancelled) {
            return onDhtGet(m, cancelled);
        });
}

std::optional<NodeInformation::Node> Dht::getSuccessor(NodeInformation::id_type key)
{
    capnp::EzRpcClient client(m_nodeInformation->getIp(), m_nodeInformation->getPort());
    auto cap = client.getMain<Peer>();
    auto &waitScope = client.getWaitScope();
    auto req = cap.getSuccessorRequest();
    req.setId(capnp::Data::Builder(kj::heapArray<kj::byte>(key.begin(), key.end())));
    return req.send().then([](capnp::Response<Peer::GetSuccessorResults> &&response) {
        return PeerImpl::nodeFromReader(response.getNode());
    }, [](const kj::Exception &) {
        return std::optional<NodeInformation::Node>{};
    }).wait(waitScope);
}

std::vector<uint8_t> Dht::onDhtPut(const api::Message_DHT_PUT &message_data, std::atomic_bool &cancelled)
{
    // TODO

    std::cout << "[DHT] DHT_PUT" << std::endl;

    std::cout
        << "[DHT.put] size:        " << std::to_string(message_data.m_header.size) << std::endl
        << "[DHT.put] type:        " << std::to_string(message_data.m_header.msg_type) << std::endl
        << "[DHT.put] ttl:         " << std::to_string(message_data.m_headerExtend.ttl) << std::endl
        << "[DHT.put] replication: " << std::to_string(message_data.m_headerExtend.replication) << std::endl
        << "[DHT.put] reserved:    " << std::to_string(message_data.m_headerExtend.reserved) << std::endl;

    // Hashing received key to convert it into length of 20 bytes
    std::string sKey{message_data.key.begin(), message_data.key.end()};
    NodeInformation::id_type finalHashedKey = NodeInformation::hash_sha1(sKey);

    auto ttl = message_data.m_headerExtend.ttl > 0
               ? std::chrono::seconds(message_data.m_headerExtend.ttl)
               : std::chrono::system_clock::duration::max();

    std::cout << "[DHT.put] getSuccessor" << std::endl;
    auto successor = getSuccessor(finalHashedKey);

    if (successor) {
        std::cout << "[DHT.put] Successor got: \"" << successor.value().getIp() << "\"" << std::endl;
        getPeerImpl().setData(*successor, message_data.key, message_data.value,
                              message_data.m_headerExtend.ttl);
    } else {
        std::cout << "[DHT.put] No Successor got!" << std::endl;
    }

    for (uint8_t i{0}; !cancelled && i < 10; ++i)
        std::this_thread::sleep_for(1s);
    return message_data.m_bytes;
}

std::vector<uint8_t> Dht::onDhtGet(const api::Message_KEY &message_data, std::atomic_bool &cancelled)
{
    // TODO

    (void) cancelled;

    std::cout << "[DHT] DHT_GET" << std::endl;

    std::cout
        << "[DHT.get] size:        " << std::to_string(message_data.m_header.size) << std::endl
        << "[DHT.get] type:        " << std::to_string(message_data.m_header.msg_type) << std::endl;

    // Hashing received key to convert it into length of 20 bytes
    std::string sKey{message_data.key.begin(), message_data.key.end()};
    NodeInformation::id_type finalHashedKey = NodeInformation::hash_sha1(sKey);

    std::cout << "[DHT.get] getSuccessor" << std::endl;
    auto successor = getSuccessor(finalHashedKey);

    std::optional<std::vector<uint8_t>> response{};

    if (successor) {
        std::cout << "[DHT.get] Successor got: " << successor->getIp() << ":" << successor->getPort()
                  << std::endl;
        response = getPeerImpl().getData(*successor, message_data.key);
    } else {
        std::cout << "[DHT.get] No Successor got!" << std::endl;
    }

    if (response) {
        return api::Message_DHT_SUCCESS(message_data.key, *response);
    } else {
        return api::Message_KEY(util::constants::DHT_FAILURE, message_data.key);
    }
}


/*
 *
 *
 *
 */




void Dht::create()
{
    std::cout << "[PEER.create]" << std::endl;
    m_nodeInformation->setPredecessor();
    m_nodeInformation->setSuccessor(m_nodeInformation->getNode());
}

void Dht::join(const NodeInformation::Node &node)
{
    std::cout << "[PEER.join]" << std::endl;
    m_nodeInformation->setPredecessor();

    capnp::EzRpcClient client{node.getIp(), node.getPort()};
    auto cap = client.getMain<Peer>();
    auto req = cap.getSuccessorRequest();
    req.setId(capnp::Data::Builder(
        kj::heapArray<kj::byte>(m_nodeInformation->getId().begin(), m_nodeInformation->getId().end())));

    return req.send().then([this](capnp::Response<Peer::GetSuccessorResults> &&response) {
        std::cout << "[PEER.join] got response" << std::endl;
        auto successor = PeerImpl::nodeFromReader(response.getNode());
        m_nodeInformation->setSuccessor(successor);
    }, [](const kj::Exception &e) {
        std::cout << "[PEER.join] Exception in request" << std::endl << e.getDescription().cStr() << std::endl;
    }).wait(client.getWaitScope());
}

void Dht::stabilize()
{
    auto successor = m_nodeInformation->getSuccessor();
    if (!successor) {
        // std::cout << "[PEER.stabilize] no successor" << std::endl;
        return;
    }

    capnp::EzRpcClient client{successor->getIp(), successor->getPort()};
    auto cap = client.getMain<Peer>();
    auto req = cap.getPredecessorRequest();

    auto predOfSuccessor = req.send().then([](capnp::Response<Peer::GetPredecessorResults> &&response) {
        auto predOfSuccessor = PeerImpl::nodeFromReader(response.getNode());
        if (!predOfSuccessor) {
            // std::cout << "[PEER.stabilize] closest preceding empty response" << std::endl;
        }
        return predOfSuccessor;
    }, [](const kj::Exception &e) {
        std::cout << "[PEER.stabilize] Exception in request" << std::endl << e.getDescription().cStr() << std::endl;
        return std::optional<NodeInformation::Node>{};
    }).wait(client.getWaitScope());

    if (predOfSuccessor && util::is_in_range_loop(
        predOfSuccessor->getId(), m_nodeInformation->getId(), successor->getId(),
        false, false
    )) {
        m_nodeInformation->setSuccessor(predOfSuccessor);
        capnp::EzRpcClient client2{predOfSuccessor->getIp(), predOfSuccessor->getPort()};
        auto cap2 = client2.getMain<Peer>();
        auto req2 = cap2.notifyRequest();
        PeerImpl::buildNode(req2.getNode(), m_nodeInformation->getNode());
        return req2.send().then([](capnp::Response<Peer::NotifyResults> &&) {
            // std::cout << "[PEER.stabilize] got response" << std::endl;
        }, [](const kj::Exception &e) {
            std::cout << "[PEER.stabilize] connection issue with predecessor of successor" << std::endl
                      << e.getDescription().cStr() << std::endl;
        }).wait(client2.getWaitScope());
    } else {
        capnp::EzRpcClient client2{successor->getIp(), successor->getPort()};
        auto cap2 = client2.getMain<Peer>();
        auto req2 = cap2.notifyRequest();
        PeerImpl::buildNode(req2.getNode(), m_nodeInformation->getNode());
        return req2.send().then([](capnp::Response<Peer::NotifyResults> &&) {
            // std::cout << "[PEER.stabilize] got response" << std::endl;
        }, [](const kj::Exception &e) {
            std::cout << "[PEER.stabilize] connection issue with successor" << std::endl
                      << e.getDescription().cStr() << std::endl;
        }).wait(client2.getWaitScope());
    }
}

void Dht::fixFingers()
{
    auto successor = getSuccessor(m_nodeInformation->getId() +
                                  util::pow2<uint8_t, SHA_DIGEST_LENGTH>(nextFinger));
    m_nodeInformation->setFinger(
        nextFinger,
        successor);
    nextFinger = (nextFinger + 1) % NodeInformation::key_bits;
}

void Dht::checkPredecessor()
{
    if (!m_nodeInformation->getPredecessor())
        return;

    capnp::EzRpcClient client{
        m_nodeInformation->getPredecessor()->getIp(), m_nodeInformation->getPredecessor()->getPort()
    };
    auto cap = client.getMain<Peer>();
    auto req = cap.getPredecessorRequest(); // This request doesn't matter, it is used as a ping
    return req.send().then([](capnp::Response<Peer::GetPredecessorResults> &&) {
        // std::cout << "[PEER.checkPredecessor] got response" << std::endl;
    }, [this](const kj::Exception &) {
        // std::cout << "[PEER.checkPredecessor] Exception in request" << std::endl << e.getDescription().cStr() << std::endl;
        // Delete predecessor
        m_nodeInformation->setPredecessor();
    }).wait(client.getWaitScope());
}
