#include "NodeInformation.h"

NodeInformation::NodeInformation()
{
    setMIp("127.0.0.1");
    setMPort(40000);
    setMSha1NodeId(FindSha1Key(m_node.getIp() + ":" + std::to_string(m_node.getPort())));
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