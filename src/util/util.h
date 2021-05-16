#ifndef DHT_UTIL_H
#define DHT_UTIL_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include <cctype>
#include <string>
#include <optional>

namespace util
{
    inline uint16_t swapBytes16(uint16_t x)
    {
        return static_cast<uint16_t>(((0x00ffu & x) << 8) | ((0xff00u & x) >> 8));
    }

    inline uint32_t swapBytes32(uint32_t x)
    {
        return ((0x000000ffu & x) << 24) |
               ((0x0000ff00u & x) << 8) |
               ((0x00ff0000u & x) >> 8) |
               ((0xff000000u & x) >> 24);
    }

    inline uint64_t swapBytes64(uint64_t x)
    {
        return ((0x00000000000000ffull & x) << 56) |
               ((0x000000000000ff00ull & x) << 40) |
               ((0x0000000000ff0000ull & x) << 24) |
               ((0x00000000ff000000ull & x) << 8) |
               ((0x000000ff00000000ull & x) >> 8) |
               ((0x0000ff0000000000ull & x) >> 24) |
               ((0x00ff000000000000ull & x) >> 40) |
               ((0xff00000000000000ull & x) >> 56);
    }

    template<typename T>
    std::vector<std::byte> convertToBytes(const T &bytes, std::optional<std::size_t> size = {})
    {
        std::vector<std::byte> ret(size ? size.value() : bytes.size());
        std::transform(bytes.begin(), size ? bytes.begin() + (std::min(size.value(), bytes.size())) : bytes.end(), ret.begin(), [](char c) { return std::byte(c); });
        return ret;
    }

    void hexdump(const std::vector<std::byte> &bytes, std::size_t stride = 16);
} // namespace util

#endif //DHT_UTIL_H
