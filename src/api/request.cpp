#include "request.h"

#include <iostream>
#include <utility>
#include <vector>

using namespace api;

Request::Request(MessageHeader header, std::vector<uint8_t> bytes) :
    m_rawBytes(std::move(bytes))
{
    if (m_rawBytes.size() < header.size)
        throw bad_buffer_size("buffer smaller than specified in header");

    if (header.msg_type == util::constants::DHT_PUT) {
        m_decodedData = std::make_unique<Message_DHT_PUT>(m_rawBytes);
    } else if (header.msg_type == util::constants::DHT_GET) {
        m_decodedData = std::make_unique<Message_KEY>(m_rawBytes);
    } else if (header.msg_type == util::constants::DHT_PUT_KEY_IS_HASH_OF_DATA) {
        m_decodedData = std::make_unique<Message_DHT_PUT_KEY_IS_HASH_OF_DATA>(m_rawBytes);
    } else {
        throw bad_request("message type incorrect: " + std::to_string(header.msg_type));
    }
}

Request::Request(Request &&other) noexcept
{
    if (this != &other) {
        m_rawBytes = std::move(other.m_rawBytes);
        m_decodedData = std::move(other.m_decodedData);
    }
}

Request &Request::operator=(Request &&other) noexcept
{
    if (this != &other) {
        m_rawBytes = std::move(other.m_rawBytes);
        m_decodedData = std::move(other.m_decodedData);
    }
    return *this;
}

std::vector<uint8_t> Request::getBytes() const
{
    return m_rawBytes;
}