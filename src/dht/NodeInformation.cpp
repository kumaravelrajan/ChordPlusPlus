//
// Created by kumar on 30-05-2021.
//
#include "NodeInformation.h"

NodeInformation::NodeInformation()
{
    setMIp("127.0.0.1");
    setMPort(40000);
    setMSha1NodeId(FindSha1Key(m_node.getIp() + ":" + std::to_string(m_node.getPort())));
}

std::array<uint8_t, SHA_DIGEST_LENGTH> NodeInformation::FindSha1Key(const std::string &str)
{
    unsigned char obuf[SHA_DIGEST_LENGTH] = {};

    SHA1(reinterpret_cast<const unsigned char *>(str.c_str()), str.length(), obuf);

    std::array<uint8_t, SHA_DIGEST_LENGTH> ret{};
    std::copy(obuf, obuf + SHA_DIGEST_LENGTH, ret.begin());
    return ret;
}

// Getters and Setters
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
std::array<uint8_t, SHA_DIGEST_LENGTH> NodeInformation::getMSha1NodeId() const
{
    return m_node.getId();
}
void NodeInformation::setMSha1NodeId(const std::array<uint8_t, SHA_DIGEST_LENGTH> &mSha1NodeId)
{
    m_node.setId(mSha1NodeId);
}

void NodeInformation::Node::setIp(std::string ip)
{ m_ip = std::move(ip); }
void NodeInformation::Node::setPort(uint16_t port)
{ m_port = port; }
void NodeInformation::Node::setId(std::array<uint8_t, SHA_DIGEST_LENGTH> id)
{ m_id = id; }

std::string NodeInformation::Node::getIp() const
{ return m_ip; }
uint16_t NodeInformation::Node::getPort() const
{ return m_port; }
std::array<uint8_t, SHA_DIGEST_LENGTH> NodeInformation::Node::getId() const
{ return m_id; }