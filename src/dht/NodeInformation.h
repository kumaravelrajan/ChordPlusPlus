#ifndef DHT_NODEINFORMATION_H
#define DHT_NODEINFORMATION_H

#include <string>
#include <openssl/sha.h>
#include <iostream>

#define SHA1_CONSIDERATION_LIMIT 9

// using namespace std; // This is the greatest sin you can commit

class NodeInformation
{
private:
    std::string m_Key;
    std::string m_Value;
    std::string m_Ip;
    std::string m_Port;
    std::string m_Sha1_nodeId;

public:
    [[nodiscard]] const std::string &getMSha1NodeId() const;
    void setMSha1NodeId(const std::string &mSha1NodeId);

public:
    [[nodiscard]] const std::string &getMValue() const;
    void setMValue(const std::string &mValue);
    [[nodiscard]] const std::string &getMIp() const;
    void setMIp(const std::string &mIp);
    [[nodiscard]] const std::string &getMPort() const;
    void setMPort(const std::string &mPort);
    [[nodiscard]] const std::string &getMKey() const;
    void setMKey(const std::string &mKey);

public:
    // Constructor
    NodeInformation();

    // Methods
    void FindSha1Key(const std::string &key);

    // Getters and setters
};

#endif //DHT_NODEINFORMATION_H
