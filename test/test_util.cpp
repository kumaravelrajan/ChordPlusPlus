#include <iostream>
#include <cstdint>
#include "assertions.h"
#include "util.h"

int main()
{
    return run_test([]() {
        assert_equal(0x3412, util::swapBytes16(0x1234));
        assert_equal(0x0001, util::swapBytes16(0x0100));

        assert_equal(0x78563412, util::swapBytes32(0x12345678));
        assert_equal(0x04030201, util::swapBytes32(0x01020304));

        assert_equal(0xf0debc9a78563412ull, util::swapBytes64(0x123456789abcdef0ull));
        assert_equal(0x0807060504030201ull, util::swapBytes64(0x0102030405060708ull));

        return 0;
    });
}