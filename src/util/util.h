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
#include <cctype>
#include <sstream>
#include <iomanip>
#include <bitset>
#include <openssl/sha.h>
#include <array>

namespace util
{
    namespace detail
    {
        // To allow ADL with custom begin/end
        using std::begin;
        using std::end;

        template<typename T>
        auto is_iterable_impl(int)
        -> decltype(begin(std::declval<T &>()) != end(std::declval<T &>()), // begin/end and operator !=
            void(), // Handle evil operator ,
            ++std::declval<decltype(begin(std::declval<T &>())) &>(), // operator ++
            void(*begin(std::declval<T &>())), // operator*
            std::true_type{});

        template<typename T>
        std::false_type is_iterable_impl(...);
    }

    template<typename T>
    using is_iterable = decltype(detail::is_iterable_impl<T>(0));

    template<typename T>
    constexpr bool is_iterable_v = is_iterable<T>::value;

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

    void hexdump(const std::vector<uint8_t> &bytes, std::size_t stride = 16, std::ostream &os = std::cout);

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
        return (min_value <= max_value && (min_value != max_value || (include_min && include_max))) ?
               ((include_min ? (value >= min_value) : (value > min_value)) &&
                (include_max ? (value <= max_value) : (value < max_value))) :
               ((include_min ? (value >= min_value) : (value > min_value)) ||
                (include_max ? (value <= max_value) : (value < max_value)));
    }

    template<typename T, size_t size, std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
    constexpr auto pow2(size_t exp)
    {
        std::array<T, size> ret{T{0}};
        int64_t index = size - 1 - exp / 8;
        if (index < size && index >= 0)
            ret[index] = 1 << (exp % 8);
        return ret;
    }

    template<typename Char>
    inline auto to_lower(const std::basic_string<Char> &str)
    {
        std::basic_string<Char> ret;
        std::transform(str.begin(), str.end(), std::back_inserter(ret), [](Char c) { return std::tolower(c); });
        return ret;
    }

    template<typename Char>
    inline auto join(const std::vector<std::basic_string<Char>> &str, const std::string &delimiter)
    {
        std::basic_string<Char> ret;
        for (auto it = str.begin(); it != str.end(); ++it) {
            if (it == str.begin())
                ret = *it;
            else
                ret = ret + delimiter + *it;
        }
        return ret;
    }

    template<typename T>
    inline std::string to_string(const T &val)
    {
        std::ostringstream os{};
        os << val;
        return os.str();
    }

    template<typename TO>
    inline TO from_string(const std::string &str)
    {
        TO ret{};
        std::istringstream{str} >> ret;
        return std::move(ret);
    }

    template<typename C, std::enable_if_t<is_iterable_v<C>, int> = 0>
    inline std::string
    hexdump(const C &bytes, uint16_t stride = 16, bool print_offsets = true, bool print_readable = true)
    {
        std::ostringstream ss{};
        size_t i, j;
        ss << std::setfill('0') << std::right << std::hex;
        for (i = 0; i < bytes.size(); i += stride) {
            if (i != 0)
                ss << std::endl;
            if (print_offsets)
                ss << std::setw(6) << i << ": ";

            for (j = 0; j < stride; j++) {
                if (j > 0)
                    ss << " ";

                if (i + j < bytes.size())
                    ss << std::setw(2) << static_cast<unsigned>(bytes[i + j]);
                else
                    ss << "  ";
            }
            if (print_readable) {
                ss << " ";
                for (j = 0; j < stride; j++)
                    if (i + j < bytes.size())
                        ss << (std::isprint(static_cast<int>(bytes[i + j])) ? static_cast<char>(bytes[i + j]) : '.');
            }
        }
        return ss.str();
    }

    template<typename C, std::enable_if_t<is_iterable_v<C>, int> = 0>
    inline std::string
    bytedump(const C &bytes, uint16_t stride = 16)
    {
        std::ostringstream ss{};
        size_t i, j;
        ss << std::setfill('0') << std::right;
        for (i = 0; i < bytes.size(); i += stride) {
            if (i != 0)
                ss << std::endl;

            for (j = 0; j < stride; j++) {
                if (j > 0)
                    ss << " ";

                if (i + j < bytes.size())
                    ss << std::setw(1) << std::bitset<8>{static_cast<unsigned>(bytes[i + j])};
                else
                    ss << "  ";
            }
        }
        return ss.str();
    }

    // Sha256 online calculators provide 64 character hashes - 2 characters = 1 byte.
    // Byte dump of such a hash.
    template<typename C, std::enable_if_t<is_iterable_v<C>, int> = 0>
    inline std::string
    HexDump64ToHexDump32(const C &bytes)
    {
        uint8_t HexDump32ToReturn[32];
        
        size_t i, j = 0;
        std::stringstream ss{};
        ss << std::setfill('0') << std::right;
        
        for (i = 0; i < bytes.size() - 1; i += 2) {
            std::string str;
            str.push_back(bytes[i]);
            str.push_back(bytes[i+1]);
            ss << std::hex << str;
        }

        std::string sample = ss.str();
        const char *pos = &sample[0];
        uint8_t toReturn[32];

        for (size_t count = 0; count < 32; count++) {
            sscanf(pos, "%2hhx", &toReturn[count]);
            pos += 2;
        }

        return ss.str();
    }

    template<typename T>
    inline std::array<uint8_t, SHA_DIGEST_LENGTH> hash_sha1(const T &bytes)
    {
        std::array<uint8_t, SHA_DIGEST_LENGTH> ret{};
        ::SHA1(
            reinterpret_cast<const unsigned char *>(&*bytes.cbegin()),
            bytes.size(),
            reinterpret_cast<unsigned char *>(&ret.front()));
        return ret;
    }

    template<typename T>
    inline std::array<uint8_t, SHA256_DIGEST_LENGTH> hash_sha256(const T &bytes)
    {
        std::array<uint8_t, SHA256_DIGEST_LENGTH> ret{};
        ::SHA256(
            reinterpret_cast<const unsigned char *>(&*bytes.cbegin()),
            bytes.size(),
            reinterpret_cast<unsigned char *>(&ret.front()));
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
