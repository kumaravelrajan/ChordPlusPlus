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

    struct MessageData
    {
        std::vector<uint8_t> m_bytes;
        MessageHeader m_header;

        MessageData(std::vector<uint8_t> bytes);

        virtual ~MessageData(){}
    };

    struct Message_KEY_VALUE: MessageData
    {
        std::vector<uint8_t> key, value;

        Message_KEY_VALUE(std::vector<uint8_t> bytes);
        Message_KEY_VALUE(uint16_t msg_type, const std::vector<uint8_t> &key, const std::vector<uint8_t> &value);
    };

    struct Message_KEY: MessageData
    {
        std::vector<uint8_t> key;

        Message_KEY(std::vector<uint8_t> bytes);
        Message_KEY(uint16_t msg_type, const std::vector<uint8_t> &key);
    };

    template<uint16_t>
    struct message_type_from_int
    {
    };

    template<>
    struct message_type_from_int<util::constants::DHT_GET>
    {
        using type = Message_KEY;
    };

    template<>
    struct message_type_from_int<util::constants::DHT_PUT>
    {
        using type = Message_KEY_VALUE;
    };

    template<uint16_t i>
    using message_type_from_int_t = typename message_type_from_int<i>::type;
} // namespace API

#endif //DHT_REQUEST_DATA_H
