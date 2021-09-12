#include "message_data.h"
#include <iostream>
#include <exception>

api::MessageHeader::MessageHeader(uint16_t size, uint16_t msg_type) :
    size(size), msg_type(msg_type) {}

api::MessageHeader::MessageHeader(const MessageHeaderRaw &raw) :
    size(util::swapBytes16(raw.size)),
    msg_type(util::swapBytes16(raw.msg_type)) {}

api::MessageHeader::operator MessageHeaderRaw() const
{
    return {
        .size = util::swapBytes16(size),
        .msg_type = util::swapBytes16(msg_type)
    };
}

api::MessageHeaderExtend::MessageHeaderExtend(uint16_t ttl, uint8_t replication, uint8_t reserved) :
    ttl(ttl), replication(replication), reserved(reserved) {}

api::MessageHeaderExtend::MessageHeaderExtend(const MessageHeaderRaw &raw) :
    ttl(util::swapBytes16(raw.ttl)),
    replication(raw.replication), reserved(raw.reserved) {}

api::MessageHeaderExtend::operator MessageHeaderRaw() const
{
    return {
        .ttl = util::swapBytes16(ttl),
        .replication = replication,
        .reserved = reserved
    };
}

api::MessageData::MessageData(std::vector<uint8_t> bytes) :
    m_bytes(std::move(bytes)),
    m_header(MessageHeader(reinterpret_cast<MessageHeader::MessageHeaderRaw &>(m_bytes[0]))) {}

api::Message_DHT_PUT::Message_DHT_PUT(std::vector<uint8_t> bytes) :
    MessageData(std::move(bytes)),
    m_headerExtend(MessageHeaderExtend(
        reinterpret_cast<MessageHeaderExtend::MessageHeaderRaw &>(m_bytes[sizeof(MessageHeader::MessageHeaderRaw)]))),
    key(m_bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw) + sizeof(MessageHeaderExtend::MessageHeaderRaw),
        m_bytes.begin() +
        (sizeof(MessageHeader::MessageHeaderRaw) + sizeof(MessageHeaderExtend::MessageHeaderRaw) + 32)),
    value(m_bytes.begin() +
          (sizeof(MessageHeader::MessageHeaderRaw) + sizeof(MessageHeaderExtend::MessageHeaderRaw) + 32),
          m_bytes.begin() + m_header.size)
{
    if (m_header.size < sizeof(MessageHeader::MessageHeaderRaw) + sizeof(MessageHeaderExtend::MessageHeaderRaw) + 32)
        throw std::runtime_error("Key is too small!");
}

api::Message_DHT_PUT::Message_DHT_PUT(const std::vector<uint8_t> &key, const std::vector<uint8_t> &value,
                                      uint16_t ttl, uint8_t replication) :
    MessageData(std::vector<uint8_t>(
        sizeof(MessageHeader::MessageHeaderRaw) + sizeof(MessageHeaderExtend::MessageHeaderRaw) + key.size() +
        value.size()))
{
    m_header = MessageHeader(static_cast<uint16_t>(m_bytes.size()), util::constants::DHT_PUT);
    m_headerExtend = MessageHeaderExtend(ttl, replication, 0);
    reinterpret_cast<MessageHeader::MessageHeaderRaw &>(m_bytes[0]) = MessageHeader::MessageHeaderRaw(m_header);
    reinterpret_cast<MessageHeaderExtend::MessageHeaderRaw &>(m_bytes[sizeof(MessageHeader::MessageHeaderRaw)]) =
        MessageHeaderExtend::MessageHeaderRaw(m_headerExtend);
    std::copy(
        key.begin(), key.end(),
        m_bytes.begin() +
        sizeof(MessageHeader::MessageHeaderRaw) +
        sizeof(MessageHeaderExtend::MessageHeaderRaw)
    );
    std::copy(
        value.begin(), value.end(),
        m_bytes.begin() + (static_cast<int>(
            sizeof(MessageHeader::MessageHeaderRaw) +
            sizeof(MessageHeaderExtend::MessageHeaderRaw))
        ) + static_cast<int>(key.size())
    );
}

api::Message_DHT_PUT_KEY_IS_HASH_OF_DATA::Message_DHT_PUT_KEY_IS_HASH_OF_DATA(std::vector<uint8_t> bytes) :
    MessageData(std::move(bytes)),
    m_headerExtend(MessageHeaderExtend(
        reinterpret_cast<MessageHeaderExtend::MessageHeaderRaw &>(m_bytes[sizeof(MessageHeader::MessageHeaderRaw)]))),
    value(m_bytes.begin() +
    (sizeof(MessageHeader::MessageHeaderRaw) + sizeof(MessageHeaderExtend::MessageHeaderRaw)),
    m_bytes.begin() + m_header.size)
{
    key = value;

    if (m_header.size < sizeof(MessageHeader::MessageHeaderRaw) + sizeof(MessageHeaderExtend::MessageHeaderRaw))
        throw std::runtime_error("Message is too small!");
}

api::Message_DHT_SUCCESS::Message_DHT_SUCCESS(std::vector<uint8_t> bytes) :
    MessageData(std::move(bytes)),
    key(m_bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw) + sizeof(MessageHeaderExtend::MessageHeaderRaw),
        m_bytes.begin() +
        (sizeof(MessageHeader::MessageHeaderRaw) + sizeof(MessageHeaderExtend::MessageHeaderRaw) + 32)),
    value(m_bytes.begin() +
          (sizeof(MessageHeader::MessageHeaderRaw) + sizeof(MessageHeaderExtend::MessageHeaderRaw) + 32),
          m_bytes.begin() + m_header.size)
{
    if (m_header.size < sizeof(MessageHeader::MessageHeaderRaw) + 32)
        throw std::runtime_error("Key is too small!");
}

api::Message_DHT_SUCCESS::Message_DHT_SUCCESS(const std::vector<uint8_t> &key, const std::vector<uint8_t> &value) :
    MessageData(std::vector<uint8_t>(sizeof(MessageHeader::MessageHeaderRaw) + key.size() + value.size()))
{
    m_header = MessageHeader(static_cast<uint16_t>(m_bytes.size()), util::constants::DHT_SUCCESS);
    reinterpret_cast<MessageHeader::MessageHeaderRaw &>(m_bytes[0]) = MessageHeader::MessageHeaderRaw(m_header);
    std::copy(
        key.begin(), key.end(),
        m_bytes.begin() +
        sizeof(MessageHeader::MessageHeaderRaw)
    );
    std::copy(
        value.begin(), value.end(),
        m_bytes.begin() +
        static_cast<int>(sizeof(MessageHeader::MessageHeaderRaw)) +
        static_cast<int>(key.size())
    );
}

api::Message_KEY::Message_KEY(std::vector<uint8_t> bytes) :
    MessageData(std::move(bytes)),
    key(m_bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw),
        m_bytes.begin() + (sizeof(MessageHeader::MessageHeaderRaw) + 32))
{
    if (m_header.size < sizeof(MessageHeader::MessageHeaderRaw) + 32)
        throw std::runtime_error("Key is too small!");
}

api::Message_KEY::Message_KEY(uint16_t msg_type, const std::vector<uint8_t> &key) :
    MessageData(std::vector<uint8_t>(sizeof(MessageHeader::MessageHeaderRaw) + key.size()))
{
    m_header = MessageHeader(static_cast<uint16_t>(m_bytes.size()), msg_type);
    reinterpret_cast<MessageHeader::MessageHeaderRaw &>(m_bytes[0]) = MessageHeader::MessageHeaderRaw(m_header);
    std::copy(key.begin(), key.end(), m_bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw));
}

api::Message_DHT_GET_KEY_IS_HASH_OF_DATA::Message_DHT_GET_KEY_IS_HASH_OF_DATA(std::vector<uint8_t> bytes) :
MessageData(std::move(bytes)),
key(m_bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw),
    m_bytes.end())
{
    if (m_header.size < sizeof(MessageHeader::MessageHeaderRaw))
        throw std::runtime_error("Key is too small!");
}

api::Message_DHT_GET_KEY_IS_HASH_OF_DATA::Message_DHT_GET_KEY_IS_HASH_OF_DATA(uint16_t msg_type, const std::vector<uint8_t> &key) :
MessageData(std::vector<uint8_t>(sizeof(MessageHeader::MessageHeaderRaw) + key.size()))
{
    m_header = MessageHeader(static_cast<uint16_t>(m_bytes.size()), msg_type);
    reinterpret_cast<MessageHeader::MessageHeaderRaw &>(m_bytes[0]) = MessageHeader::MessageHeaderRaw(m_header);
    std::copy(key.begin(), key.end(), m_bytes.begin() + sizeof(MessageHeader::MessageHeaderRaw));
}