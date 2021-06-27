#include "NodeInformation.h"

using namespace std::chrono_literals;

NodeInformation::NodeInformation()
{
    setMSha1NodeId(FindSha1Key(m_node.getIp() + ":" + std::to_string(m_node.getPort())));
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
            std::this_thread::sleep_for(30s);
        }
    });
}

NodeInformation::NodeInformation(uint16_t portForNode)
{
    // Port set from DHT-4.cpp's StartDHT to obtain different hashes for different nodes.
    m_node.setPort(portForNode);

    setMSha1NodeId(FindSha1Key(m_node.getIp() + ":" + std::to_string(m_node.getPort())));
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
          std::this_thread::sleep_for(30s);
      }
    });
}

NodeInformation::~NodeInformation()
{
    m_destroyed = true;
    m_dataCleaner.wait();
}

NodeInformation::id_type NodeInformation::FindSha1Key(const std::string &str)
{
    unsigned char obuf[SHA_DIGEST_LENGTH] = {};

    SHA1(reinterpret_cast<const unsigned char *>(str.c_str()), str.length(), obuf);

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
std::string NodeInformation::getMIp() const
{
    return m_node.getIp();
}
void NodeInformation::setMIp(const std::string &mIp)
{
    m_node.setIp(mIp);
}
uint16_t NodeInformation::getMPort() const
{
    return m_node.getPort();
}
void NodeInformation::setMPort(uint16_t mPort)
{
    m_node.setPort(mPort);
}
NodeInformation::id_type NodeInformation::getMSha1NodeId() const
{
    return m_node.getId();
}
void NodeInformation::setMSha1NodeId(const id_type &mSha1NodeId)
{
    m_node.setId(mSha1NodeId);
}

const std::optional<NodeInformation::Node> &NodeInformation::getFinger(size_t index)
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
    std::shared_lock f{m_predecessorMutex};
    return m_fingerTable[0];
}

void NodeInformation::setSuccessor(const std::optional<Node> &node)
{
    std::unique_lock f{m_predecessorMutex};
    m_fingerTable[0] = node;
}


const std::optional<NodeInformation::Node> &NodeInformation::getPredecessor()
{
    std::shared_lock f{m_predecessorMutex};
    return m_predecessor;
}

void NodeInformation::setPredecessor(const std::optional<Node> &node)
{
    std::unique_lock f{m_predecessorMutex};
    m_predecessor = node;
}

std::optional<std::vector<uint8_t>> NodeInformation::getData(const std::vector<uint8_t> &key)
{
    std::shared_lock l{m_dataMutex};
    return m_data.contains(key) ? m_data[key].first : std::optional<std::vector<uint8_t>>{};
}

void NodeInformation::setData(const std::vector<uint8_t> &key, const std::vector<uint8_t> &value,
                              std::chrono::system_clock::duration ttl)
{
    std::unique_lock l{m_dataMutex};
    m_data[key] = std::make_pair(value, ttl == std::chrono::system_clock::duration::max()
                                        ? std::chrono::system_clock::time_point::max() :
                                        std::chrono::system_clock::now() + ttl);
}
std::pair<std::string, uint16_t> NodeInformation::getBootstrapNodeAddress()
{
    return m_bootstrapNodeAddress;
}
void NodeInformation::setBootstrapNodeAddress(std::pair<std::string, uint16_t> bootstrapNodeAddress)
{
    m_bootstrapNodeAddress = bootstrapNodeAddress;
}

// Node Methods:

void NodeInformation::Node::setIp(std::string ip)
{ m_ip = std::move(ip); }
void NodeInformation::Node::setPort(uint16_t port)
{ m_port = port; }
void NodeInformation::Node::setId(id_type id)
{ m_id = id; }

std::string NodeInformation::Node::getIp() const
{ return m_ip; }
uint16_t NodeInformation::Node::getPort() const
{ return m_port; }
NodeInformation::id_type NodeInformation::Node::getId() const
{ return m_id; }