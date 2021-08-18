#include "NodeInformation.h"
#include <centralLogControl.h>
#include <util.h>
#include <spdlog/fmt/chrono.h>

using namespace std::chrono_literals;

/* Initialize static variable */
std::vector<uint8_t> NodeInformation::m_allReplicationIndices;

NodeInformation::NodeInformation(std::string host, uint16_t port) : m_node(std::move(host), port)
{
    m_dataCleaner = std::async(std::launch::async, [this]() {
        while (!m_destroyed) {
            {
                std::unique_lock l{m_dataMutex};
                for (auto it = m_data.begin(); it != m_data.end();) {
                    auto tp = std::chrono::system_clock::now();
                    if (it->second.second < tp) {
                        it = m_data.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            std::unique_lock lock(m_cv_m);
            m_cv.wait_for(lock, 30s, [this] { return m_destroyed == true; });
        }
    });
}

NodeInformation::~NodeInformation()
{
    m_destroyed = true;
    m_cv.notify_all();
    m_dataCleaner.wait();
}

NodeInformation::id_type NodeInformation::hash_sha1(const std::string &str)
{
    unsigned char obuf[SHA_DIGEST_LENGTH] = {};

    ::SHA1(reinterpret_cast<const unsigned char *>(str.c_str()), str.length(), obuf);

    id_type ret{};
    std::copy(obuf, obuf + SHA_DIGEST_LENGTH, ret.begin());
    return ret;
}

// Getters and Setters
const NodeInformation::Node &NodeInformation::getNode() const
{
    return m_node;
}
void NodeInformation::setNode(const Node &node)
{
    m_node = node;
}
std::string NodeInformation::getIp() const
{
    return m_node.getIp();
}
void NodeInformation::setIp(const std::string &mIp)
{
    m_node.setIp(mIp);
}
uint16_t NodeInformation::getPort() const
{
    return m_node.getPort();
}
void NodeInformation::setPort(uint16_t mPort)
{
    m_node.setPort(mPort);
}
NodeInformation::id_type NodeInformation::getId() const
{
    return m_node.getId();
}
const std::optional<NodeInformation::Node> &NodeInformation::getFinger(size_t index) const
{
    std::shared_lock l{m_fingerTableMutex};
    if (index >= m_fingerTable.size())
        throw std::out_of_range("index out of bounds");
    return m_fingerTable[index];
}
void NodeInformation::setFinger(size_t index, const std::optional<Node> &node)
{
    std::unique_lock l{m_fingerTableMutex};
    if (index >= m_fingerTable.size())
        throw std::out_of_range("index out of bounds");
    m_fingerTable[index] = node;
}
const std::optional<NodeInformation::Node> &NodeInformation::getSuccessor()
{
    std::shared_lock f{m_fingerTableMutex};
    for (auto &finger : m_fingerTable)
        if (finger) return finger;
    return m_fingerTable[0];
}
void NodeInformation::setSuccessor(const std::optional<Node> &node)
{
    std::unique_lock f{m_fingerTableMutex};
    m_fingerTable[0] = node;
}
const std::optional<NodeInformation::Node> &NodeInformation::getPredecessor() const
{
    std::shared_lock f{m_predecessorMutex};
    return m_predecessor;
}
void NodeInformation::setPredecessor(const std::optional<Node> &node)
{
    std::unique_lock f{m_predecessorMutex};
    m_predecessor = node;
}
std::optional<std::vector<uint8_t>> NodeInformation::getData(const std::vector<uint8_t> &key) const
{
    std::shared_lock l{m_dataMutex};
    return m_data.contains(key) ? m_data.at(key).first : std::optional<std::vector<uint8_t>>{};
}
void NodeInformation::setData(const std::vector<uint8_t> &key, const std::vector<uint8_t> &value,
                              std::chrono::system_clock::duration ttl)
{
    setDataExpires(key, value, ttl == std::chrono::system_clock::duration::max()
                               ? std::chrono::system_clock::time_point::max() :
                               std::chrono::system_clock::now() + ttl);
}
void NodeInformation::setDataExpires(const std::vector<uint8_t> &key, const std::vector<uint8_t> &value,
                                     std::chrono::system_clock::time_point expires)
{
    SPDLOG_INFO(
        "setting data, key length: {}, value length: {}, expires: {}",
        key.size(), value.size(), expires
    );
    std::unique_lock l{m_dataMutex};
    m_data[key] = std::make_pair(value, expires);
}
std::optional<NodeInformation::Node> NodeInformation::getBootstrapNode() const
{
    return m_bootstrapNodeAddress;
}
void NodeInformation::setBootstrapNode(const std::optional<Node> &bootstrapNodeAddress)
{
    m_bootstrapNodeAddress = bootstrapNodeAddress;
}
std::optional<NodeInformation::data_type> NodeInformation::getDataItemsForNodeId(
    const Node &newNode
) const
{
    std::shared_lock l{m_dataMutex};
    NodeInformation::data_type dataToReturn;
    id_type new_id = newNode.getId();
    auto pred = getPredecessor();
    id_type pred_id = pred ? pred->getId() : new_id;
    for (auto &s : m_data) {
        const std::string strDataKey{s.first.begin(), s.first.end()};
        id_type key_hash = NodeInformation::hash_sha1(strDataKey);

        if (util::is_in_range_loop(key_hash, pred_id, new_id, false, true)) {
            dataToReturn.insert(s);
        }
    }
    return dataToReturn;
}

NodeInformation::data_type NodeInformation::getAllDataInNode() const
{
    std::shared_lock l{m_dataMutex};
    return m_data;
}
void NodeInformation::deleteDataAssignedToPredecessor(std::vector<std::vector<uint8_t>> &keyOfDataItemsToDelete)
{
    std::unique_lock l{m_dataMutex};
    for(auto s : keyOfDataItemsToDelete){
        m_data.erase(s);
    }
}
void NodeInformation::setReplicationIndex(const uint8_t &replicationIndex){
    NodeInformation::m_allReplicationIndices.push_back(replicationIndex);
}
std::optional<uint8_t> NodeInformation::getAverageReplicationIndex() const
{
    return static_cast<uint8_t>((std::accumulate(NodeInformation::m_allReplicationIndices.begin(), NodeInformation::m_allReplicationIndices.end(), static_cast<uint8_t>(0))) / NodeInformation::m_allReplicationIndices.size());
}

// Node Methods:

void NodeInformation::Node::setIp(std::string ip)
{
    m_ip = std::move(ip);
    id_valid = false;
}
void NodeInformation::Node::setPort(uint16_t port)
{
    m_port = port;
    id_valid = false;
}

std::string NodeInformation::Node::getIp() const { return m_ip; }
uint16_t NodeInformation::Node::getPort() const { return m_port; }
NodeInformation::id_type NodeInformation::Node::getId() const
{
    if (!id_valid) updateId();
    return m_id;
}

void NodeInformation::Node::updateId() const
{
    id_valid = true;
    m_id = hash_sha1(m_ip + ":" + std::to_string(m_port));
}