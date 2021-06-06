#include "request.h"

#include <iostream>
#include <utility>

using namespace api;

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

std::vector<std::byte> Request::getBytes() const
{
    return m_rawBytes;
}
