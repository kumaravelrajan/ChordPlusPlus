//
// Created by kumar on 30-05-2021.
//
#include "NodeInformation.h"

NodeInformation::NodeInformation() {
m_Value = "";
m_Ip = "127.0.0.1";
m_Port = "40000";
m_Key = m_Ip + ":" + m_Port;
}

void NodeInformation::FindSha1Key(string Key)
{
    unsigned char obuf[SHA_DIGEST_LENGTH] = { };

    // convert string to an unsigned char array because SHA1 takes unsigned char array as parameter
    unsigned char unsigned_key[Key.length()] = {};
    for(int i = 0;i < Key.length(); i++)
    {
        unsigned_key[i] = Key[i];
    }

    // Invoke OpenSSL SHA1 method
    SHA1(unsigned_key,sizeof(unsigned_key),obuf);

    char cFinalHash[SHA_DIGEST_LENGTH * 3 + 1] = {};

    // Tracker for snprintf bytes
    int n = 0;

    // Convert uchar array to char array
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        n += snprintf(cFinalHash + n, SHA_DIGEST_LENGTH, "%u", obuf[i]);
    }

    string sFinalHash(cFinalHash);
    sFinalHash = sFinalHash.substr(0, SHA1_CONSIDERATION_LIMIT);
}

// Getters and Setters
const string &NodeInformation::getMValue() const
{
    return m_Value;
}
void NodeInformation::setMValue(const string &mValue)
{
    m_Value = mValue;
}
const string &NodeInformation::getMIp() const
{
    return m_Ip;
}
void NodeInformation::setMIp(const string &mIp)
{
    m_Ip = mIp;
}
const string &NodeInformation::getMPort() const
{
    return m_Port;
}
void NodeInformation::setMPort(const string &mPort)
{
    m_Port = mPort;
}
const string &NodeInformation::getMKey() const
{
    return m_Key;
}
void NodeInformation::setMKey(const string &mKey)
{
    m_Key = mKey;
}
const string &NodeInformation::getMSha1NodeId() const
{
    return m_Sha1_nodeId;
}
void NodeInformation::setMSha1NodeId(const string &mSha1NodeId)
{
    m_Sha1_nodeId = mSha1NodeId;
}
