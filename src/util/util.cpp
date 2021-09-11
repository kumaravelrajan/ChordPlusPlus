#include "util.h"
#include <algorithm>
#include <iostream>
#include <iomanip>

void util::hexdump(const std::vector<uint8_t> &bytes, size_t stride, std::ostream &os)
{
    size_t i, j;
    auto old = os.flags();
    os << std::setfill('0') << std::right << std::hex;
    for (i = 0; i < bytes.size(); i += stride) {
        os << std::setw(6) << i << ": ";
        for (j = 0; j < stride; j++)
            if (i + j < bytes.size())
                os << std::setw(2) << static_cast<unsigned>(bytes[i + j]) << " ";
            else
                os << "   ";
        os << " ";
        for (j = 0; j < stride; j++)
            if (i + j < bytes.size())
                os << (std::isprint(static_cast<int>(bytes[i + j])) ? static_cast<char>(bytes[i + j]) : '.');
        os << std::endl;
    }
    os.flags(old);
}
