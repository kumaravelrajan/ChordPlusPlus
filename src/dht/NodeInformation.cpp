#include "NodeInformation.h"
#include <centralLogControl.h>
#include <util.h>
#include <spdlog/fmt/chrono.h>

using namespace std::chrono_literals;

/* Initialize static variables */
std::deque<uint8_t> NodeInformation::m_allReplicationIndices;
uint8_t NodeInformation::m_difficulty = DEFAULT_DIFFICULTY;
uint8_t NodeInformation::m_replicationLimit = DEFAULT_REPLICATION_LIMIT;

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

// Getters and Setters
NodeInformation::Node NodeInformation::getNode() const
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
void NodeInformation::setId(std::optional<id_type> id)
{
    m_node.setId(id);
}
std::optional<NodeInformation::Node> NodeInformation::getFinger(size_t index) const
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
std::optional<NodeInformation::Node> NodeInformation::getSuccessor()
{
    std::shared_lock f{m_fingerTableMutex};
    for (auto &finger: m_fingerTable)
        if (finger) return finger;
    return m_fingerTable[0];
}
void NodeInformation::setSuccessor(const std::optional<Node> &node)
{
    std::unique_lock f{m_fingerTableMutex};
    m_fingerTable[0] = node;
}
std::optional<NodeInformation::Node> NodeInformation::getPredecessor() const
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
    // TODO: Use Howard Hinnant's Date library
    auto t = std::chrono::system_clock::now().time_since_epoch();
    std::tm tm{};
    tm.tm_hour = static_cast<int>(std::chrono::duration_cast<std::chrono::hours>(t).count() % 24);
    tm.tm_min = static_cast<int>(std::chrono::duration_cast<std::chrono::minutes>(t).count() % 60);
    tm.tm_sec = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(t).count() % 60);
    SPDLOG_INFO(
        "setting data, key length: {}, value length: {}, expires: UTC-{:%H:%M:%S}",
        key.size(), value.size(), tm
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
    for (auto &s: m_data) {
        const std::string strDataKey{s.first.begin(), s.first.end()};
        id_type key_hash = util::hash_sha256(strDataKey);

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
    for (const auto &s: keyOfDataItemsToDelete) {
        m_data.erase(s);
    }
}
void NodeInformation::setReplicationIndex(const uint8_t &replicationIndex)
{
    if(NodeInformation::m_allReplicationIndices.size() < DEFAULT_NUM_OF_REPLICATION_TO_CALCULATE_AVERAGE){
        NodeInformation::m_allReplicationIndices.push_back(replicationIndex);
    } else {
        NodeInformation::m_allReplicationIndices.pop_front();
        NodeInformation::m_allReplicationIndices.push_back(replicationIndex);
    }
}
std::optional<uint8_t> NodeInformation::getAverageReplicationIndex()
{
    if (!m_allReplicationIndices.empty()) {
        return static_cast<uint8_t>((std::accumulate(NodeInformation::m_allReplicationIndices.begin(),
                                                     NodeInformation::m_allReplicationIndices.end(),
                                                     static_cast<uint8_t>(0))) /
                                    NodeInformation::m_allReplicationIndices.size());
    } else {
        return static_cast<uint8_t>(0);
    }
}
uint8_t NodeInformation::getDifficulty()
{
    return m_difficulty;
}
void NodeInformation::setDifficulty(uint8_t &DifficultyToSet)
{
    if (m_difficulty == DEFAULT_DIFFICULTY) {
        m_difficulty = DifficultyToSet;
    }
}
uint8_t NodeInformation::getReplicationLimitOnEachNode() const{
    return m_replicationLimit;
}
void NodeInformation::setReplicationLimitOnEachNode(uint8_t &replicationLimit){
    if(m_replicationLimit == DEFAULT_REPLICATION_LIMIT){
        m_replicationLimit = replicationLimit;
    }
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
void NodeInformation::Node::setId(std::optional<id_type> id)
{
    m_explicit_id = id;
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
    if (m_explicit_id)
        m_id = *m_explicit_id;
    else
        m_id = util::hash_sha256(m_ip + ":" + std::to_string(m_port));
}