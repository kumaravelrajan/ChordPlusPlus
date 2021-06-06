//
// Created by kumar on 30-05-2021.
//

#ifndef DHT_NODEINFORMATION_H
#define DHT_NODEINFORMATION_H

# include <string>
# include <openssl/sha.h>
#include <iostream>

#define SHA1_CONSIDERATION_LIMIT 9

using namespace std;

class NodeInformation{
private:
    string m_Key;
    string m_Value;
    string m_Ip;
    string m_Port;
    string m_Sha1_nodeId;

public:
    const string &getMSha1NodeId() const;
    void setMSha1NodeId(const string &mSha1NodeId);

public:
    const string &getMValue() const;
    void setMValue(const string &mValue);
    const string &getMIp() const;
    void setMIp(const string &mIp);
    const string &getMPort() const;
    void setMPort(const string &mPort);
    const string &getMKey() const;
    void setMKey(const string &mKey);

public:
    // Constructor
    NodeInformation();

    // Methods
    void FindSha1Key(string key);

    // Getters and setters

};

#endif //DHT_NODEINFORMATION_H
