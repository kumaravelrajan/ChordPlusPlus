#ifndef DHT_NODEINFORMATION_H
#define DHT_NODEINFORMATION_H

#include <string>
#include <openssl/sha.h>
#include <array>
#include <vector>
#include <cstdint>
#include <shared_mutex>
#include <optional>
#include <map>
#include <future>
#include <numeric>

#define SHA1_CONSIDERATION_LIMIT 9


/**
 * @brief Stores the dynamic state of the Peer. Needs to be thread-safe.
 */
class NodeInformation
{
public:
    static constexpr size_t key_bits = SHA_DIGEST_LENGTH * 8;
    using id_type = std::array<uint8_t, SHA_DIGEST_LENGTH>;
    using data_type = std::map<std::vector<uint8_t>, std::pair<std::vector<uint8_t>, std::chrono::system_clock::time_point>>;

    class Node
    {
        std::string m_ip;
        uint16_t m_port;

        mutable bool id_valid{true};
        mutable id_type m_id{0};
    public:
        explicit Node(std::string ip = "127.0.0.1", uint16_t port = 6969)
            : m_ip(std::move(ip)), m_port(port) { updateId(); }

        Node(std::string ip, uint16_t port, id_type id)
            : m_ip(std::move(ip)), m_port(port), m_id(id) {}

        void setIp(std::string ip);
        void setPort(uint16_t port);

        [[nodiscard]] std::string getIp() const;
        [[nodiscard]] uint16_t getPort() const;
        [[nodiscard]] id_type getId() const;

        void updateId() const;

        bool operator==(const Node &other) const { return m_ip == other.m_ip && m_port == other.m_port; }
        bool operator!=(const Node &other) const { return !(*this == other); }
    };

private:
    Node m_node;
    std::vector<std::optional<Node>> m_fingerTable{key_bits}; // m = number of bits in the id
    mutable std::shared_mutex m_fingerTableMutex{};
    std::optional<Node> m_predecessor{};
    mutable std::shared_mutex m_predecessorMutex{};
    /// Stores data along with the expiry date.
    data_type m_data{};
    mutable std::shared_mutex m_dataMutex{};
    /// Asynchronously removes expired data entries.
    std::future<void> m_dataCleaner{};
    std::atomic_bool m_destroyed{false};
    mutable std::mutex m_cv_m{};
    std::condition_variable m_cv{};
    /// Bootstrap node details
    std::optional<Node> m_bootstrapNodeAddress{};
    uint8_t m_replicationIndex{};

    /* Used for DHT GET */
    static std::vector<uint8_t> m_allReplicationIndices;

public:
    [[nodiscard]] const Node &getNode() const;
    void setNode(const Node &node);
    [[nodiscard]] id_type getId() const;
    [[nodiscard]] std::string getIp() const;
    void setIp(const std::string &mIp);
    [[nodiscard]] uint16_t getPort() const;
    void setPort(uint16_t mPort);
    [[nodiscard]] std::optional<Node> getBootstrapNode() const;
    void setBootstrapNode(const std::optional<Node> &);
    [[nodiscard]] std::optional<uint8_t> getReplicationIndex() const;
    void setReplicationIndex(const uint8_t &replicationIndex);
    [[nodiscard]] std::optional<uint8_t> getAverageReplicationIndex() const;

    /**
     * @throws std::out_of_range
     */
    [[nodiscard]] const std::optional<Node> &getFinger(size_t index) const;
    /**
     * @throws std::out_of_range
     */
    void setFinger(size_t index, const std::optional<Node> &node = {});

    [[nodiscard]] const std::optional<Node> &getSuccessor();
    void setSuccessor(const std::optional<Node> &node = {});
    [[nodiscard]] const std::optional<Node> &getPredecessor() const;
    void setPredecessor(const std::optional<Node> &node = {});

    [[nodiscard]] std::optional<std::vector<uint8_t>> getData(const std::vector<uint8_t> &key) const;
    void setData(const std::vector<uint8_t> &key, const std::vector<uint8_t> &value,
                 std::chrono::system_clock::duration ttl = std::chrono::system_clock::duration::max());
    void setDataExpires(const std::vector<uint8_t> &key, const std::vector<uint8_t> &value,
                        std::chrono::system_clock::time_point expires);

    [[nodiscard]] std::optional<NodeInformation::data_type> getDataItemsForNodeId(const Node &newNode) const;
    [[nodiscard]] NodeInformation::data_type getAllDataInNode() const;
    [[nodiscard]] void deleteDataAssignedToPredecessor(std::vector<std::vector<uint8_t>> &keyOfDataItemsToDelete);
public:
    // Constructor
    explicit NodeInformation(std::string host = "", uint16_t port = 0);
    ~NodeInformation();

    // Methods
    static id_type hash_sha1(const std::string &str);

    // Getters and setters
};

#endif //DHT_NODEINFORMATION_H
