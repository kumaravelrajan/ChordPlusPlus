#include "request.h"

#include <iostream>
#include <utility>
#include <vector>

using namespace api;

template<class T, std::enable_if_t<std::is_convertible_v<std::remove_cvref_t<T>, std::vector<uint8_t>>, int>>
Request::Request(T &&bytes):
    m_rawBytes(std::forward<T>(bytes))
{
    if (m_rawBytes.size() < sizeof(MessageHeader::MessageHeaderRaw))
        throw bad_buffer_size("buffer too small for header");

    MessageHeader header(reinterpret_cast<MessageHeader::MessageHeaderRaw &>(m_rawBytes[0]));

    if (m_rawBytes.size() < header.size)
        throw bad_buffer_size("buffer smaller than specified in header");

    if (header.msg_type == util::constants::DHT_PUT) {
        m_decodedData = std::make_unique<Message_DHT_PUT>(m_rawBytes);
    } else if (header.msg_type == util::constants::DHT_GET) {
        m_decodedData = std::make_unique<Message_DHT_GET>(m_rawBytes);
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

template Request::Request(std::vector<uint8_t> &&);
template Request::Request(std::vector<uint8_t> &);
template Request::Request(const std::vector<uint8_t> &);