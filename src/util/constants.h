#ifndef DHT_UTIL_CONSTANTS_H
#define DHT_UTIL_CONSTANTS_H

#include <cstdint>

namespace util
{
    namespace constants
    {
        constexpr uint16_t DHT_PUT { 650 };
        constexpr uint16_t DHT_GET { 651 };
        constexpr uint16_t DHT_SUCCESS { 652 };
        constexpr uint16_t DHT_FAILURE { 653 };
    } // namespace Constants
} // namespace util

#endif //DHT_UTIL_CONSTANTS_H
