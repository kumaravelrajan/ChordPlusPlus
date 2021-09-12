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
#include <deque>

// Also declared in config.h.
#define DEFAULT_DIFFICULTY 1
#define DEFAULT_REPLICATION_LIMIT 5
#define DEFAULT_NUM_OF_REPLICATION_TO_CALCULATE_AVERAGE 50


/**
 * @brief Stores the dynamic state of the Peer. Needs to be thread-safe.
 */
class NodeInformation
{
public:
    static constexpr size_t key_bits = SHA256_DIGEST_LENGTH * 8;
    using id_type = std::array<uint8_t, key_bits / 8>;
    using data_type = std::map<std::vector<uint8_t>, std::pair<std::vector<uint8_t>, std::chrono::system_clock::time_point>>;

    class Node
    {
    public:
        struct Node_hash
        {
            /**
             * @brief
             * interpret the array as if it consisted of size_t's by adding elements with increasing offsets.
             * Once one size_t's worth of bytes was added, the result is xor-ed into the result.
             * It is as if all the size_t's were xor-ed together.
             * The important part is that if id.size() is not divisible by 8, this will still use all bytes.
             */
            std::size_t operator()(const Node &node) const
            {
                auto id = node.getId();
                std::size_t ret = 0;
                std::size_t tmp = 0;
                for (uint32_t i = 0; i < id.size(); ++i) {
                    tmp += static_cast<std::size_t>(id[i]) << (8 * (i % sizeof(std::size_t)));
                    if ((i + 1) % sizeof(std::size_t) == 0) {
                        ret ^= tmp;
                        tmp = 0;
                    }
                }
                ret ^= tmp;
                return ret;
            }
        };

    private:
        std::string m_ip;
        uint16_t m_port;

        mutable bool id_valid{true};
        mutable id_type m_id{0};

        std::optional<id_type> m_explicit_id{};
    public:
        explicit Node(std::string ip = "127.0.0.1", uint16_t port = 6969)
            : m_ip(std::move(ip)), m_port(port) { updateId(); }

        Node(std::string ip, uint16_t port, id_type id)
            : m_ip(std::move(ip)), m_port(port), m_id(id), m_explicit_id(id) {}

        void setIp(std::string ip);
        void setPort(uint16_t port);
        void setId(std::optional<id_type> id = {});

        [[nodiscard]] std::string getIp() const;
        [[nodiscard]] uint16_t getPort() const;
        [[nodiscard]] id_type getId() const;

    private:
        void updateId() const;

    public:
        bool operator==(const Node &other) const { return getId() == other.getId(); }
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

    /* Used for DHT GET */
    static std::deque<uint8_t> m_allReplicationIndices;

    // Used for PoW
    static uint8_t m_difficulty;

    // Used for maintaining limit on replication of one data item on each node.
    // If m_replicationLimit = 5, the same data item can be replicated on the same node a maximum of 10 times.
    static uint8_t m_replicationLimit;

public:
    [[nodiscard]] Node getNode() const;
    void setNode(const Node &node);
    [[nodiscard]] id_type getId() const;
    void setId(std::optional<id_type> id = {});
    [[nodiscard]] std::string getIp() const;
    void setIp(const std::string &mIp);
    [[nodiscard]] uint16_t getPort() const;
    void setPort(uint16_t mPort);
    [[nodiscard]] std::optional<Node> getBootstrapNode() const;
    void setBootstrapNode(const std::optional<Node> &);
    static void setReplicationIndex(const uint8_t &replicationIndex);
    [[nodiscard]] static std::optional<uint8_t> getAverageReplicationIndex();
    [[nodiscard]] static uint8_t getDifficulty();
    static void setDifficulty(uint8_t &DifficultyToSet);
    uint8_t getReplicationLimitOnEachNode() const;
    void setReplicationLimitOnEachNode(uint8_t &replicationLimit);

    /**
     * @throws std::out_of_range
     */
    [[nodiscard]] std::optional<Node> getFinger(size_t index) const;
    /**
     * @throws std::out_of_range
     */
    void setFinger(size_t index, const std::optional<Node> &node = {});

    [[nodiscard]] std::optional<Node> getSuccessor();
    void setSuccessor(const std::optional<Node> &node = {});
    [[nodiscard]] std::optional<Node> getPredecessor() const;
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

    // Getters and setters
};

#endif //DHT_NODEINFORMATION_H
