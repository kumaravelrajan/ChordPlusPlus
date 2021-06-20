#ifndef DHT_NODEINFORMATION_H
#define DHT_NODEINFORMATION_H

#include <string>
#include <openssl/sha.h>
#include <array>
#include <vector>
#include <cstdint>
#include <iostream>
#include <shared_mutex>
#include <optional>

#define SHA1_CONSIDERATION_LIMIT 9

/**
 * @brief Stores the dynamic state of the Peer. Needs to be thread-safe.
 */
class NodeInformation
{
public:
    using id_type = std::array<uint8_t, SHA_DIGEST_LENGTH>;

    class Node
    {
        std::string m_ip;
        uint16_t m_port;
        id_type m_id;
    public:
        explicit Node(std::string ip = "127.0.0.1", uint16_t port = 6969, id_type id = {0})
            :
            m_ip(std::move(ip)), m_port(port), m_id(id)
        {}

        void setIp(std::string ip);
        void setPort(uint16_t port);
        void setId(id_type id);

        [[nodiscard]] std::string getIp() const;
        [[nodiscard]] uint16_t getPort() const;
        [[nodiscard]] id_type getId() const;
    };

private:
    Node m_node;
    std::vector<std::optional<Node>> m_fingerTable{SHA_DIGEST_LENGTH * 8}; // m = number of bits in the id
    std::shared_mutex m_fingerTableMutex{};
    std::optional<Node> m_predecessor{};
    std::shared_mutex m_predecessorMutex{};

public:
    [[nodiscard]] const Node &getNode() const;
    void setNode(const Node &node);
    [[nodiscard]] id_type getMSha1NodeId() const;
    void setMSha1NodeId(const id_type &mSha1NodeId);
    [[nodiscard]] std::string getMIp() const;
    void setMIp(const std::string &mIp);
    [[nodiscard]] uint16_t getMPort() const;
    void setMPort(uint16_t mPort);

    /**
     * @throws std::out_of_range
     */
    [[nodiscard]] const std::optional<Node> &getFinger(size_t index);
    /**
     * @throws std::out_of_range
     */
    void setFinger(size_t index, const std::optional<Node> &node);

    [[nodiscard]] const std::optional<Node> &getPredecessor();
    void setPredecessor(const std::optional<Node> &node);

public:
    // Constructor
    NodeInformation();

    // Methods
    static id_type FindSha1Key(const std::string &str);

    // Getters and setters
};

#endif //DHT_NODEINFORMATION_H
