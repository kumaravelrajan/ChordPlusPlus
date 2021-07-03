#ifndef DHT_API_REQUEST_DATA_H
#define DHT_API_REQUEST_DATA_H

#include "constants.h"
#include <vector>
#include <cstdint>
#include <util.h>

namespace api
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

    struct MessageHeaderExtend
    {
#pragma pack(push, 2)
        struct MessageHeaderRaw
        {
            uint16_t ttl;
            uint8_t replication, reserved;
        };
#pragma pack(pop)

        uint16_t ttl{0};
        uint8_t replication{0}, reserved{0};

        MessageHeaderExtend() = default;
        MessageHeaderExtend(uint16_t ttl, uint8_t replication, uint8_t reserved);
        explicit MessageHeaderExtend(const MessageHeaderRaw &raw);

        explicit operator MessageHeaderRaw() const;
    };

    struct MessageData
    {
        std::vector<uint8_t> m_bytes;
        MessageHeader m_header;

        MessageData(std::vector<uint8_t> bytes);

        virtual ~MessageData() = default;
    };

    struct Message_DHT_PUT : MessageData
    {
        MessageHeaderExtend m_headerExtend{};

        std::vector<uint8_t> key, value;

        Message_DHT_PUT(std::vector<uint8_t> bytes);
        Message_DHT_PUT(const std::vector<uint8_t> &key, const std::vector<uint8_t> &value,
                        uint16_t ttl = 0, uint8_t replication = 0);
    };

    struct Message_DHT_GET : MessageData
    {
        std::vector<uint8_t> key;

        Message_DHT_GET(std::vector<uint8_t> bytes);
        Message_DHT_GET(uint16_t msg_type, const std::vector<uint8_t> &key);
    };

    template<uint16_t>
    struct message_type_from_int
    {
    };

    template<>
    struct message_type_from_int<util::constants::DHT_GET>
    {
        using type = Message_DHT_GET;
    };

    template<>
    struct message_type_from_int<util::constants::DHT_PUT>
    {
        using type = Message_DHT_PUT;
    };

    template<uint16_t i>
    using message_type_from_int_t = typename message_type_from_int<i>::type;
} // namespace API

#endif //DHT_REQUEST_DATA_H
