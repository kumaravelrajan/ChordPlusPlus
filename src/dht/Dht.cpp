#include "Dht.h"
#include <util.h>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/ez-rpc.h>
#include "logging/centralLogControl.h"

#ifndef LOG_ERR
#define LOG_ERR(e) LOG_WARN("Exception in request:\n\t\t{}", e.getDescription().cStr())
#endif

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

    // from kj/async.h - Use `kj::getCurrentThreadExecutor()` to get an executor that schedules calls on the current
    // thread's event loop.
    // Here we make sure every node runs on its own thread which has its own event loop.
    m_executor.emplace(kj::getCurrentThreadExecutor());

    // It seems like promises are atomic in the event loop. So no *actual* asynchronous execution... Threads it is then
    auto f = std::async(std::launch::async, [this]() {
        mainLoop();
    });

    while (!m_mainLoopExited) {
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

    SPDLOG_TRACE("Main Loop Entered");

    std::this_thread::sleep_for(5s);

    while (true) {
        if (m_dhtCancelled) break;
        if (!m_nodeInformation->getSuccessor()) {
            if (m_nodeInformation->getBootstrapNode() &&
                m_nodeInformation->getBootstrapNode() != m_nodeInformation->getNode()) {
                join(*m_nodeInformation->getBootstrapNode());
            } else {
                create();
            }
        }

        if (m_dhtCancelled) break;
        stabilize();
        if (m_dhtCancelled) break;
        fixFingers();
        if (m_dhtCancelled) break;
        checkPredecessor();
        if (m_dhtCancelled) break;

        // Wait one second
        std::this_thread::sleep_for(1s);
    }

    SPDLOG_TRACE("Exiting Main Loop");
    m_mainLoopExited = true;
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
    SPDLOG_INFO(
        "DHT PUT\n"
        "\t\tsize:        {}\n"
        "\t\ttype:        {}\n"
        "\t\tttl:         {}\n"
        "\t\treplication: {}\n"
        "\t\treserved:    {}",
        std::to_string(message_data.m_header.size),
        std::to_string(message_data.m_header.msg_type),
        std::to_string(message_data.m_headerExtend.ttl),
        std::to_string(message_data.m_headerExtend.replication),
        std::to_string(message_data.m_headerExtend.reserved)
    );

    // Setting replication index value
    m_nodeInformation->setReplicationIndex(message_data.m_headerExtend.replication);

    // Hashing received key to convert it into length of 20 bytes
    std::string sKey{message_data.key.begin(), message_data.key.end()};
    NodeInformation::id_type finalHashedKey = NodeInformation::hash_sha1(sKey);

    auto ttl = message_data.m_headerExtend.ttl > 0
               ? std::chrono::seconds(message_data.m_headerExtend.ttl)
               : std::chrono::system_clock::duration::max();

    auto successor = getSuccessor(finalHashedKey);

    if (successor) {
        SPDLOG_DEBUG("Successor found: {}:{}", successor->getIp(), successor->getPort());
        getPeerImpl().setData(*successor, message_data.key, message_data.value,
                              message_data.m_headerExtend.ttl);
    } else {
        SPDLOG_DEBUG("No Successor found!");
    }

    /* If replicationIndex == 0 or 1, no replication done. replicationIndex defines
     * on how many nodes the value should be stored, ignoring a value of 0 */
    if (message_data.m_headerExtend.replication >= 2) {

        m_replicationFuture = std::async(std::launch::async, [this, message_data, successor]() {
            std::vector<NodeInformation::Node> nodesHavingReplicatedData{};

            for (int i = 0; i < message_data.m_headerExtend.replication - 1; ++i) {
                auto tempMessage_Data = message_data;
                tempMessage_Data.key.push_back(static_cast<uint8_t>(i + 1));

                // Hashing received key to convert it into length of 20 bytes
                std::string sKey { tempMessage_Data.key.begin(), tempMessage_Data.key.end() };
                NodeInformation::id_type finalHashedKey = NodeInformation::hash_sha1(sKey);
                auto replicationSuccessor = getSuccessor(finalHashedKey);

                if (replicationSuccessor &&
                    (std::end(nodesHavingReplicatedData) == std::find(nodesHavingReplicatedData.begin(), nodesHavingReplicatedData.end(), replicationSuccessor)) &&
                    replicationSuccessor != successor) {
                        nodesHavingReplicatedData.push_back(*replicationSuccessor);
                        getPeerImpl().setData(*replicationSuccessor, tempMessage_Data.key, message_data.value, message_data.m_headerExtend.ttl);
                }
            }
        });
    }

    for (uint8_t i{0}; !cancelled && i < 10; ++i)
        std::this_thread::sleep_for(1s);
    return message_data.m_bytes;
}

std::vector<uint8_t> Dht::onDhtGet(const api::Message_KEY &message_data, std::atomic_bool &cancelled)
{
    (void) cancelled;

    SPDLOG_INFO(
        "DHT GET\n"
        "\t\tsize:        {}\n"
        "\t\ttype:        {}",
        std::to_string(message_data.m_header.size),
        std::to_string(message_data.m_header.msg_type)
    );

    // Hashing received key to convert it into length of 20 bytes
    std::string sKey{message_data.key.begin(), message_data.key.end()};
    NodeInformation::id_type finalHashedKey = NodeInformation::hash_sha1(sKey);

    auto successor = getSuccessor(finalHashedKey);

    std::optional<std::vector<uint8_t>> response{};

    if (successor) {
        SPDLOG_DEBUG("Successor found: {}:{}", successor->getIp(), successor->getPort());
        response = getPeerImpl().getData(*successor, message_data.key);
    }
    
    if(!response && m_nodeInformation->getAverageReplicationIndex() >= 1 ) {
        /* Check if any of the replicated copies are present. */
        for (int i = 1; i <= m_nodeInformation->getAverageReplicationIndex(); ++i) {
            auto tempMessage_Data = message_data;
            tempMessage_Data.key.push_back(static_cast<uint8_t>(i));

            // Hashing received key to convert it into length of 20 bytes
            std::string sReplicatedKey { tempMessage_Data.key.begin(), tempMessage_Data.key.end() };
            NodeInformation::id_type finalReplicatedHashedKey = NodeInformation::hash_sha1(sReplicatedKey);
            auto replicationSuccessor = getSuccessor(finalReplicatedHashedKey);

            if (replicationSuccessor) {
                SPDLOG_DEBUG("Successor found for replicated data: {}:{}", replicationSuccessor->getIp(), replicationSuccessor->getPort());
                response = m_peerImpl.value().get().getData(*replicationSuccessor, tempMessage_Data.key);
                break;
            }
        }
    } else {
        SPDLOG_DEBUG("No Successor found!");
    }

    if (response) {
        return api::Message_DHT_SUCCESS(message_data.key, *response);
    } else {
        return api::Message_KEY(util::constants::DHT_FAILURE, message_data.key);
    }
}


// ===============================
// ======[ DHT MAINTENANCE ]======
// ===============================


void Dht::create()
{
    SPDLOG_TRACE("");
    m_nodeInformation->setPredecessor();
    m_nodeInformation->setSuccessor(m_nodeInformation->getNode());
}

void Dht::join(const NodeInformation::Node &node)
{
    LOG_GET
    m_nodeInformation->setPredecessor();

    capnp::EzRpcClient client{node.getIp(), node.getPort()};
    auto cap = client.getMain<Peer>();
    auto req = cap.getPoWPuzzleOnJoinRequest();
    getPeerImpl().buildNode(req.getNewNode(), m_nodeInformation->getNode());
    
    // Start RPC - get PoW puzzle from bootstrap
    std::string sFinalResponseToPuzzle{};
    req.send().then([LOG_CAPTURE, this, &sFinalResponseToPuzzle, &node](capnp::Response<Peer::GetPoWPuzzleOnJoinResults> &&response) {
        LOG_DEBUG("got PoW puzzle from bootstrap");
        auto puzzle = response.getProofOfWorkPuzzle();
        std::string strPuzzle{puzzle};

        auto difficulty = response.getDifficulty();
        std::string strDifficulty(static_cast<size_t>(difficulty), '0');

        int i = 0;
        std::string possiblePuzzleResponse{};
        while(true){
            possiblePuzzleResponse = strPuzzle;
            possiblePuzzleResponse.append(std::to_string(i));

            std::string sHashOfPossiblePuzzleResponse = util::bytedump(NodeInformation::hash_sha1(possiblePuzzleResponse), SHA_DIGEST_LENGTH);
            sHashOfPossiblePuzzleResponse.erase(std::remove_if(sHashOfPossiblePuzzleResponse.begin(), sHashOfPossiblePuzzleResponse.end(), ::isspace), sHashOfPossiblePuzzleResponse.end());

            if(sHashOfPossiblePuzzleResponse.substr(0, static_cast<size_t>(difficulty)) == strDifficulty){
                sFinalResponseToPuzzle = sHashOfPossiblePuzzleResponse;
                break;
            }
            i++;
        }

        // Now that the puzzle is solved, make RPC call to sendProofOfWorkPuzzleResponseToBootstrap
        if(!sFinalResponseToPuzzle.empty()){
            LOG_DEBUG("{}:{} successfully solved puzzle.", this->m_nodeInformation->getIp(), this->m_nodeInformation->getPort());

            capnp::EzRpcClient client2{node.getIp(), node.getPort()};
            auto cap2 = client2.getMain<Peer>();
            auto req2 = cap2.sendPoWPuzzleResponseToBootstrapAndGetSuccessorRequest();
            getPeerImpl().buildNode(req2.getNewNode(), m_nodeInformation->getNode());
            req2.setHashOfproofOfWorkPuzzleResponse(sFinalResponseToPuzzle);
            req2.setProofOfWorkPuzzleResponse(possiblePuzzleResponse);

            req2.send().then([LOG_CAPTURE, this, &node](capnp::Response<Peer::SendPoWPuzzleResponseToBootstrapAndGetSuccessorResults> &&response2){
                if(response2.hasSuccessorOfNewNode()){
                    auto successor = this->getPeerImpl().nodeFromReader(response2.getSuccessorOfNewNode());
                    this->m_nodeInformation->setSuccessor(successor);
                } else {
                    this->m_nodeInformation->setSuccessor();
                }
            }, [LOG_CAPTURE](const kj::Exception &e) {
                LOG_ERR(e);
            }).wait(client2.getWaitScope());
        }
    }, [LOG_CAPTURE](const kj::Exception &e) {
        LOG_ERR(e);
    }).wait(client.getWaitScope());
}

void Dht::stabilize()
{
    LOG_GET
    auto successor = m_nodeInformation->getSuccessor();
    if (!successor) {
        LOG_TRACE("no successor");
        return;
    }

    capnp::EzRpcClient client{successor->getIp(), successor->getPort()};
    auto cap = client.getMain<Peer>();
    auto req = cap.getPredecessorRequest();

    /* Request pre(suc(cur)). */
    auto predOfSuccessor = req.send().then([LOG_CAPTURE](capnp::Response<Peer::GetPredecessorResults> &&response) {
        auto predOfSuccessor = PeerImpl::nodeFromReader(response.getNode());
        if (!predOfSuccessor) {
            LOG_TRACE("closest preceding empty response");
        }
        return predOfSuccessor;
    }, [LOG_CAPTURE](const kj::Exception &e) {
        LOG_DEBUG("connection issue with successor\n\t\t{}", e.getDescription().cStr());
        return std::optional<NodeInformation::Node>{};
    }).wait(client.getWaitScope());

    /* If pred(suc(cur)) == cur, no need to do further processing. */
    if (!(predOfSuccessor && predOfSuccessor->getId() == m_nodeInformation->getId())) {
        /* if ( pred(suc(cur)) [called PSC] != null ) && if ( PSC is in range (cur, suc) )
         * then PSC is successor of cur and cur is predecessor of PSC. Update accordingly. */
        if (predOfSuccessor && util::is_in_range_loop(
            predOfSuccessor->getId(), m_nodeInformation->getId(), successor->getId(),
            false, false
        )) {
            m_nodeInformation->setSuccessor(predOfSuccessor);
            capnp::EzRpcClient client2{predOfSuccessor->getIp(), predOfSuccessor->getPort()};
            auto cap2 = client2.getMain<Peer>();
            auto req2 = cap2.notifyRequest();
            PeerImpl::buildNode(req2.getNode(), m_nodeInformation->getNode());
            return req2.send().then([LOG_CAPTURE, this](capnp::Response<Peer::NotifyResults> &&) {
                LOG_TRACE("got response from predecessor of successor");
            }, [LOG_CAPTURE](const kj::Exception &e) {
                LOG_DEBUG("connection issue with predecessor of successor\n\t\t{}", e.getDescription().cStr());
            }).wait(client2.getWaitScope());
        } else {
            /* if ( pred(suc(cur)) [called PSC] == null  || ( PSC!=null && PSC not in range (cur, suc) ) )
             * then cur is predecessor of suc(cur). */
            capnp::EzRpcClient client2{successor->getIp(), successor->getPort()};
            auto cap2 = client2.getMain<Peer>();
            auto req2 = cap2.notifyRequest();
            PeerImpl::buildNode(req2.getNode(), m_nodeInformation->getNode());
            return req2.send().then([LOG_CAPTURE, this](capnp::Response<Peer::NotifyResults> &&) {
                LOG_TRACE("got response from successor");
            }, [LOG_CAPTURE](const kj::Exception &e) {
                LOG_DEBUG("connection issue with successor\n\t\t{}", e.getDescription().cStr());
            }).wait(client2.getWaitScope());
        }
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
    LOG_GET
    if (!m_nodeInformation->getPredecessor())
        return;

    capnp::EzRpcClient client{
        m_nodeInformation->getPredecessor()->getIp(), m_nodeInformation->getPredecessor()->getPort()
    };
    auto cap = client.getMain<Peer>();
    auto req = cap.getPredecessorRequest(); // This request doesn't matter, it is used as a ping
    return req.send().then([LOG_CAPTURE](capnp::Response<Peer::GetPredecessorResults> &&) {
        LOG_TRACE("got response from predecessor");
    }, [LOG_CAPTURE, this](const kj::Exception &e) {
        LOG_ERR(e);
        // Delete predecessor
        m_nodeInformation->setPredecessor();
    }).wait(client.getWaitScope());
}
