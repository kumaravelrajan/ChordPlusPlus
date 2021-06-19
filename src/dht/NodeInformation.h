#ifndef DHT_NODEINFORMATION_H
#define DHT_NODEINFORMATION_H

#include <string>
#include <openssl/sha.h>
#include <array>
#include <cstdint>
#include <iostream>

#define SHA1_CONSIDERATION_LIMIT 9

/**
 * @brief Stores the dynamic state of the Peer. Needs to be thread-safe.
 */
class NodeInformation
{
public:
    class Node
    {
        std::string m_ip;
        uint16_t m_port;
        std::array<uint8_t, SHA_DIGEST_LENGTH> m_id;
    public:
        Node(std::string ip = "127.0.0.1", uint16_t port = 6969, std::array<uint8_t, SHA_DIGEST_LENGTH> id = {0})
            :
            m_ip(std::move(ip)), m_port(port), m_id(id)
        {}

        void setIp(std::string ip);
        void setPort(uint16_t port);
        void setId(std::array<uint8_t, SHA_DIGEST_LENGTH> id);

        [[nodiscard]] std::string getIp() const;
        [[nodiscard]] uint16_t getPort() const;
        [[nodiscard]] std::array<uint8_t, SHA_DIGEST_LENGTH> getId() const;
    };

private:
    Node m_node;

public:
    [[nodiscard]] std::array<uint8_t, SHA_DIGEST_LENGTH> getMSha1NodeId() const;
    void setMSha1NodeId(const std::array<uint8_t, SHA_DIGEST_LENGTH> &mSha1NodeId);
    [[nodiscard]] std::string getMIp() const;
    void setMIp(const std::string &mIp);
    [[nodiscard]] uint16_t getMPort() const;
    void setMPort(uint16_t mPort);

public:
    // Constructor
    NodeInformation();

    // Methods
    static std::array<uint8_t, SHA_DIGEST_LENGTH> FindSha1Key(const std::string &str);

    // Getters and setters
};

#endif //DHT_NODEINFORMATION_H
