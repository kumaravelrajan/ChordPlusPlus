#ifndef DHT_API_REQUEST_DATA_H
#define DHT_API_REQUEST_DATA_H

#include <vector>
#include <cstdint>

namespace API
{
    struct RequestData
    {
        virtual std::vector<std::byte> &getRawBytes() = 0;
    };

    struct Request_DHT_PUT: RequestData
    {
        std::vector<std::byte> key, value;

        Request_DHT_PUT(std::vector<std::byte> &bytes, int headerSize, int messageSize);

        std::vector<std::byte> &getRawBytes() override;

    private:
        std::vector<std::byte> &m_bytes;
    };

    struct Request_DHT_GET: RequestData
    {
        std::vector<std::byte> key;

        Request_DHT_GET(std::vector<std::byte> &bytes, int headerSize, int messageSize);

        std::vector<std::byte> &getRawBytes() override;

    private:
        std::vector<std::byte> &m_bytes;
    };
} // namespace API

#endif //DHT_REQUEST_DATA_H
