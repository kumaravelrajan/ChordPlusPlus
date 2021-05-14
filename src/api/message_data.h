#ifndef DHT_API_REQUEST_DATA_H
#define DHT_API_REQUEST_DATA_H

#include <vector>
#include <cstdint>
#include <util.h>

namespace API
{
    struct MessageHeader
    {
#pragma pack(push, 2)
        struct MessageHeaderRaw
        {
            uint16_t size, msg_type;
        };
#pragma pack(pop)

        uint16_t size, msg_type;

        MessageHeader() = default;
        MessageHeader(uint16_t size, uint16_t msg_type);
        explicit MessageHeader(const MessageHeaderRaw &raw);

        explicit operator MessageHeaderRaw() const;
    };

    struct MessageData
    {
        std::vector<std::byte> m_bytes;

        explicit MessageData(std::vector<std::byte> bytes);

    private:
        [[maybe_unused]] virtual void VIRTUAL() {}
    };

    struct Message_KEY_VALUE: MessageData
    {
        std::vector<std::byte> key, value;

        Message_KEY_VALUE(std::vector<std::byte> bytes, const MessageHeader &header);
        Message_KEY_VALUE(uint16_t msg_type, const std::vector<std::byte> &key, const std::vector<std::byte> &value);
    };

    struct Message_KEY: MessageData
    {
        std::vector<std::byte> key;

        Message_KEY(std::vector<std::byte> bytes, const MessageHeader &header);
        Message_KEY(uint16_t msg_type, const std::vector<std::byte> &key);
    };
} // namespace API

#endif //DHT_REQUEST_DATA_H
