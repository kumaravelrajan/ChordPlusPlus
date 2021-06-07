//
// Created by kumar on 30-05-2021.
//
#include "NodeInformation.h"

NodeInformation::NodeInformation()
{
    m_Value = "";
    m_Ip = "127.0.0.1";
    m_Port = "40000";
    m_Key = m_Ip + ":" + m_Port;
}

void NodeInformation::FindSha1Key(const std::string &key)
{
    unsigned char obuf[SHA_DIGEST_LENGTH] = {};

    // Invoke OpenSSL SHA1 method
    SHA1(reinterpret_cast<const unsigned char *>(key.c_str()), key.length(), obuf);

    char cFinalHash[SHA_DIGEST_LENGTH * 3 + 1] = {};

    // Tracker for snprintf bytes
    int n = 0;

    // Convert uchar array to char array
    for (auto i : obuf) {
        n += snprintf(cFinalHash + n, SHA_DIGEST_LENGTH, "%u", i);
    }

    std::string sFinalHash(cFinalHash);
    sFinalHash = sFinalHash.substr(0, SHA1_CONSIDERATION_LIMIT);
}

// Getters and Setters
const std::string &NodeInformation::getMValue() const
{
    return m_Value;
}
void NodeInformation::setMValue(const std::string &mValue)
{
    m_Value = mValue;
}
const std::string &NodeInformation::getMIp() const
{
    return m_Ip;
}
void NodeInformation::setMIp(const std::string &mIp)
{
    m_Ip = mIp;
}
const std::string &NodeInformation::getMPort() const
{
    return m_Port;
}
void NodeInformation::setMPort(const std::string &mPort)
{
    m_Port = mPort;
}
const std::string &NodeInformation::getMKey() const
{
    return m_Key;
}
void NodeInformation::setMKey(const std::string &mKey)
{
    m_Key = mKey;
}
const std::string &NodeInformation::getMSha1NodeId() const
{
    return m_Sha1_nodeId;
}
void NodeInformation::setMSha1NodeId(const std::string &mSha1NodeId)
{
    m_Sha1_nodeId = mSha1NodeId;
}
