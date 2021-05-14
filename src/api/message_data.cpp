#include "message_data.h"

API::Request_DHT_PUT::Request_DHT_PUT(std::vector<std::byte> &bytes, const MessageHeader &header):
    m_bytes(const_cast<std::vector<std::byte> &>(bytes)),
    key(bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw), bytes.begin() + (sizeof(MessageHeader::MessageHeaderRaw) + 32)),
    value(bytes.begin() + (sizeof(MessageHeader::MessageHeaderRaw) + 32), bytes.begin() + header.size)
{
}

std::vector<std::byte> &API::Request_DHT_PUT::getRawBytes()
{
    return m_bytes;
}

API::Request_DHT_GET::Request_DHT_GET(std::vector<std::byte> &bytes, const MessageHeader &header):
    m_bytes(const_cast<std::vector<std::byte> &>(bytes)),
    key(bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw), bytes.begin() + (sizeof(MessageHeader::MessageHeaderRaw) + 32))
{
}

std::vector<std::byte> &API::Request_DHT_GET::getRawBytes()
{
    return m_bytes;
}
