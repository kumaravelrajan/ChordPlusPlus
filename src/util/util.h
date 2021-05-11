#ifndef DHT_UTIL_H
#define DHT_UTIL_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include <cctype>

namespace util
{
    inline uint16_t swapBytes16(uint16_t x)
    {
        return ((0x00ff & x) << 8) |
               ((0xff00 & x) >> 8);
    }

    inline uint32_t swapBytes32(uint32_t x)
    {
        return ((0x000000ff & x) << 24) |
               ((0x0000ff00 & x) << 8) |
               ((0x00ff0000 & x) >> 8) |
               ((0xff000000 & x) >> 24);
    }

    inline uint64_t swapBytes64(uint64_t x)
    {
        return ((0x00000000000000ff & x) << 56) |
               ((0x000000000000ff00 & x) << 40) |
               ((0x0000000000ff0000 & x) << 24) |
               ((0x00000000ff000000 & x) << 8) |
               ((0x000000ff00000000 & x) >> 8) |
               ((0x0000ff0000000000 & x) >> 24) |
               ((0x00ff000000000000 & x) >> 40) |
               ((0xff00000000000000 & x) >> 56);
    }

    void hexdump(const std::vector<std::byte> &bytes, size_t stride = 16);
} // namespace util

#endif //DHT_UTIL_H
