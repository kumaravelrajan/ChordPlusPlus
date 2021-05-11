#include "request_data.h"

API::Request_DHT_PUT::Request_DHT_PUT(std::vector<std::byte> &bytes, int headerSize, int messageSize):
    m_bytes(const_cast<std::vector<std::byte> &>(bytes)),
    key(bytes.begin() + headerSize, bytes.begin() + (headerSize + 32)),
    value(bytes.begin() + (headerSize + 32), bytes.begin() + headerSize)
{
}

API::Request_DHT_GET::Request_DHT_GET(std::vector<std::byte> &bytes, int headerSize, int messageSize):
    m_bytes(const_cast<std::vector<std::byte> &>(bytes)),
    key(bytes.begin() + headerSize, bytes.begin() + (headerSize + 32))
{
}
