#include "message_data.h"
#include <iostream>
#include <exception>

api::MessageHeader::MessageHeader(uint16_t size, uint16_t msg_type):
    size(size), msg_type(msg_type) {}

api::MessageHeader::MessageHeader(const MessageHeaderRaw &raw):
    size(util::swapBytes16(raw.size)),
    msg_type(util::swapBytes16(raw.msg_type)) {}

api::MessageHeader::operator MessageHeaderRaw() const
{
    return {
        .size = util::swapBytes16(size),
        .msg_type = util::swapBytes16(msg_type),
    };
}

api::MessageData::MessageData(std::vector<std::byte> bytes):
    m_bytes(std::move(bytes)),
    m_header(MessageHeader(reinterpret_cast<MessageHeader::MessageHeaderRaw &>(m_bytes[0]))) {}

api::Message_KEY_VALUE::Message_KEY_VALUE(std::vector<std::byte> bytes):
    MessageData(std::move(bytes)),
    key(m_bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw), m_bytes.begin() + (sizeof(MessageHeader::MessageHeaderRaw) + 32)),
    value(m_bytes.begin() + (sizeof(MessageHeader::MessageHeaderRaw) + 32), m_bytes.begin() + m_header.size)
{
    if (m_header.size < sizeof(MessageHeader::MessageHeaderRaw) + 32)
        throw std::runtime_error("Key is too small!");
}

api::Message_KEY_VALUE::Message_KEY_VALUE(uint16_t msg_type, const std::vector<std::byte> &key, const std::vector<std::byte> &value):
    MessageData(std::vector<std::byte>(sizeof(MessageHeader::MessageHeaderRaw) + key.size() + value.size()))
{
    m_header = MessageHeader(static_cast<uint16_t>(m_bytes.size()), msg_type);
    reinterpret_cast<MessageHeader::MessageHeaderRaw &>(m_bytes[0]) = MessageHeader::MessageHeaderRaw(m_header);
    std::copy(key.begin(), key.end(), m_bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw));
    std::copy(value.begin(), value.end(), m_bytes.begin() + (static_cast<int>(sizeof(MessageHeader::MessageHeaderRaw))) + static_cast<int>(key.size()));
}

api::Message_KEY::Message_KEY(std::vector<std::byte> bytes):
    MessageData(std::move(bytes)),
    key(m_bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw), m_bytes.begin() + (sizeof(MessageHeader::MessageHeaderRaw) + 32))
{
    if (m_header.size < sizeof(MessageHeader::MessageHeaderRaw) + 32)
        throw std::runtime_error("Key is too small!");
}

api::Message_KEY::Message_KEY(uint16_t msg_type, const std::vector<std::byte> &key):
    MessageData(std::vector<std::byte>(sizeof(MessageHeader::MessageHeaderRaw) + key.size()))
{
    m_header = MessageHeader(static_cast<uint16_t>(m_bytes.size()), msg_type);
    reinterpret_cast<MessageHeader::MessageHeaderRaw &>(m_bytes[0]) = MessageHeader::MessageHeaderRaw(m_header);
    std::copy(key.begin(), key.end(), m_bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw));
}