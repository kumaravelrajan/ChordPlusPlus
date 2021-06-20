#ifndef DHT_UTIL_H
#define DHT_UTIL_H

#include <algorithm>
#include <type_traits>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cctype>
#include <string>
#include <optional>
#include <iostream>
#include <InfInt.h>

namespace util
{
    constexpr uint16_t swapBytes16(uint16_t x)
    {
        return static_cast<uint16_t>(((0x00ffu & x) << 8) | ((0xff00u & x) >> 8));
    }

    constexpr uint32_t swapBytes32(uint32_t x)
    {
        return ((0x000000ffu & x) << 24) |
               ((0x0000ff00u & x) << 8) |
               ((0x00ff0000u & x) >> 8) |
               ((0xff000000u & x) >> 24);
    }

    constexpr uint64_t swapBytes64(uint64_t x)
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
    std::vector<uint8_t> convertToBytes(const T &bytes, std::optional<std::size_t> size = {})
    {
        std::vector<uint8_t> ret(size ? size.value() : bytes.size());
        std::transform(bytes.begin(),
                       size ? bytes.begin() + static_cast<int>(std::min(size.value(), bytes.size())) : bytes.end(),
                       ret.begin(), [](char c) { return uint8_t(c); });
        return ret;
    }

    void hexdump(const std::vector<uint8_t> &bytes, std::size_t stride = 16);

    template<typename F, typename... Ts>
    struct is_one_of
    {
        static constexpr bool value = (std::is_same_v<F, Ts> || ...);
    };

    template<typename... Ts>
    constexpr bool is_one_of_v = is_one_of<Ts...>::value;

    template<typename T, T F, T... Ts>
    struct t_is_one_of
    {
        static constexpr bool value = ((F == Ts) || ...);
    };

    template<typename T, T... Ts>
    constexpr bool t_is_one_of_v = t_is_one_of<T, Ts...>::value;

    template<typename T, typename U, typename V>
    constexpr bool is_in_range_loop(
        const T &value, const U &min_value, const V &max_value,
        bool include_min = true, bool include_max = false)
    {
        return (min_value <= max_value) ?
               ((include_min ? (value >= min_value) : (value > min_value)) &&
                (include_max ? (value <= max_value) : (value < max_value))) :
               ((include_min ? (value >= min_value) : (value > min_value)) ||
                (include_max ? (value <= max_value) : (value < max_value)));
    }

    template<typename T, size_t size, std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
    constexpr auto pow2(size_t exp)
    {
        std::array<T, size> ret{0};
        int64_t index = size - 1 - exp / 8;
        if (index < size && index >= 0)
            ret[index] = 1 << (exp % 8);
        return ret;
    }

    template<typename T, size_t size, std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
    InfInt arrToInfInt(const std::array<T, size> &arr)
    {
        InfInt ret{0};
        for (size_t i = 0; i < size; ++i) {
            ret = (ret * InfInt(1ull << ((sizeof(T) * 4))) * InfInt(1ull << ((sizeof(T) * 4)))) + InfInt(arr[i]);
        }
        return ret;
    }
} // namespace util

template<typename T, size_t size, std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
constexpr auto operator+(const std::array<T, size> &a, const std::array<T, size> &b)
{
    std::array<T, size> ret;
    bool carry = false;
    for (int i = size - 1; i >= 0; --i) {
        T sum = 0;
        if (carry) {
            ++sum;
            carry = false;
        }
        if (size_t(static_cast<T>(~0ull)) - size_t(sum) < size_t(a[i])) carry = true;
        sum += a[i];
        if (size_t(static_cast<T>(~0ull)) - size_t(sum) < size_t(b[i])) carry = true;
        sum += b[i];
        ret[i] = sum;
    }
    return ret;
}

#endif //DHT_UTIL_H
