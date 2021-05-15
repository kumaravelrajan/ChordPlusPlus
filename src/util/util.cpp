#include "util.h"
#include <algorithm>
#include <iostream>
#include <iomanip>

std::vector<std::byte> util::convertToBytes(const std::string &byteString)
{
    std::vector<std::byte> bytes(byteString.size());
    std::transform(byteString.begin(), byteString.end(), bytes.begin(), [](char c) { return std::byte(c); });
    return bytes;
}

void util::hexdump(const std::vector<std::byte> &bytes, size_t stride)
{
    size_t i, j;
    auto old = std::cout.flags();
    std::cout << std::setfill('0') << std::right << std::hex;
    for (i = 0; i < bytes.size(); i += stride) {
        std::cout << std::setw(6) << i << ": ";
        for (j = 0; j < stride; j++)
            if (i + j < bytes.size())
                std::cout << std::setw(2) << static_cast<unsigned>(bytes[i + j]) << " ";
            else
                std::cout << "   ";
        std::cout << " ";
        for (j = 0; j < stride; j++)
            if (i + j < bytes.size())
                std::cout << (std::isprint(static_cast<char>(bytes[i + j])) ? static_cast<char>(bytes[i + j]) : '.');
        std::cout << '\n';
    }
    std::cout.flags(old);
}