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

        MessageHeader(const MessageHeaderRaw &raw) // NOLINT(google-explicit-constructor)
        {
            size = util::swapBytes16(raw.size);
            msg_type = util::swapBytes16(raw.msg_type);
        }
    };

    struct MessageData
    {
        virtual std::vector<std::byte> &getRawBytes() = 0;
    };

    struct Request_DHT_PUT: MessageData
    {
        std::vector<std::byte> key, value;

        Request_DHT_PUT(std::vector<std::byte> &bytes, const MessageHeader &header);

        std::vector<std::byte> &getRawBytes() override;

    private:
        std::vector<std::byte> &m_bytes;
    };

    struct Request_DHT_GET: MessageData
    {
        std::vector<std::byte> key;

        Request_DHT_GET(std::vector<std::byte> &bytes, const MessageHeader &header);

        std::vector<std::byte> &getRawBytes() override;

    private:
        std::vector<std::byte> &m_bytes;
    };
} // namespace API

#endif //DHT_REQUEST_DATA_H
