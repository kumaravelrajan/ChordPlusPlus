#include "util.h"
#include <algorithm>

std::vector<std::byte> util::convertToBytes(const std::string &byteString)
{
    std::vector<std::byte> bytes(byteString.size());
    std::transform(byteString.begin(), byteString.end(), bytes.begin(), [](char c) { return std::byte(c); });
    return bytes;
}

void util::hexdump(const std::vector<std::byte> &bytes, size_t stride)
{
    size_t i, j;
    for (i = 0; i < bytes.size(); i += stride) {
        printf("%06x: ", i);
        for (j = 0; j < stride; j++)
            if (i + j < bytes.size())
                printf("%02x ", static_cast<unsigned char>(bytes[i + j]));
            else
                printf("   ");
        printf(" ");
        for (j = 0; j < stride; j++)
            if (i + j < bytes.size())
                printf("%c", std::isprint(static_cast<unsigned char>(bytes[i + j])) ? static_cast<unsigned char>(bytes[i + j]) : '.');
        printf("\n");
    }
}