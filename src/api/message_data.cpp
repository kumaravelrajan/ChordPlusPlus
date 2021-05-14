#include "message_data.h"
#include <iostream>

API::MessageHeader::MessageHeader(uint16_t size, uint16_t msg_type):
    size(size), msg_type(msg_type) {}

API::MessageHeader::MessageHeader(const MessageHeaderRaw &raw):
    size(util::swapBytes16(raw.size)),
    msg_type(util::swapBytes16(raw.msg_type)) {}

API::MessageHeader::operator MessageHeaderRaw() const
{
    return {
        .size = util::swapBytes16(size),
        .msg_type = util::swapBytes16(msg_type),
    };
}

API::MessageData::MessageData(std::vector<std::byte> bytes):
    m_bytes(std::move(bytes)) {}

API::Message_KEY_VALUE::Message_KEY_VALUE(std::vector<std::byte> bytes, const MessageHeader &header):
    MessageData(std::move(bytes)),
    key(bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw), bytes.begin() + (sizeof(MessageHeader::MessageHeaderRaw) + 32)),
    value(m_bytes.begin() + (sizeof(MessageHeader::MessageHeaderRaw) + 32), m_bytes.begin() + header.size)
{
}

API::Message_KEY_VALUE::Message_KEY_VALUE(uint16_t msg_type, const std::vector<std::byte> &key, const std::vector<std::byte> &value):
    MessageData(std::vector<std::byte>(sizeof(MessageHeader::MessageHeaderRaw) + key.size() + value.size()))
{
    MessageHeader header(static_cast<uint16_t>(m_bytes.size()), msg_type);
    reinterpret_cast<MessageHeader::MessageHeaderRaw &>(m_bytes[0]) = MessageHeader::MessageHeaderRaw(header);
    std::copy(key.begin(), key.end(), m_bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw));
    std::copy(value.begin(), value.end(), m_bytes.begin() + (static_cast<int>(sizeof(MessageHeader::MessageHeaderRaw))) + static_cast<int>(key.size()));
}

API::Message_KEY::Message_KEY(std::vector<std::byte> bytes, const MessageHeader &header):
    MessageData(std::move(bytes)),
    key(m_bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw), m_bytes.begin() + (sizeof(MessageHeader::MessageHeaderRaw) + 32))
{
}

API::Message_KEY::Message_KEY(uint16_t msg_type, const std::vector<std::byte> &key):
    MessageData(std::vector<std::byte>(sizeof(MessageHeader::MessageHeaderRaw) + key.size()))
{
    MessageHeader header(static_cast<uint16_t>(m_bytes.size()), msg_type);
    reinterpret_cast<MessageHeader::MessageHeaderRaw &>(m_bytes[0]) = MessageHeader::MessageHeaderRaw(header);
    std::copy(key.begin(), key.end(), m_bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw));
}