#include "util.h"

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