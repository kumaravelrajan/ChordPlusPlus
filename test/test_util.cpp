#include <iostream>
#include <cstdint>
#include "assertions.h"
#include <util.h>
#include <constants.h>

int main()
{
    return run_test("Util", []() {
        assert_equal(0x3412, util::swapBytes16(0x1234));
        assert_equal(0x0001, util::swapBytes16(0x0100));

        assert_equal(0x78563412, util::swapBytes32(0x12345678));
        assert_equal(0x04030201, util::swapBytes32(0x01020304));

        assert_equal(0xf0debc9a78563412ull, util::swapBytes64(0x123456789abcdef0ull));
        assert_equal(0x0807060504030201ull, util::swapBytes64(0x0102030405060708ull));

        assert_equal(650, util::constants::DHT_PUT);
        assert_equal(651, util::constants::DHT_GET);
        assert_equal(652, util::constants::DHT_SUCCESS);
        assert_equal(653, util::constants::DHT_FAILURE);

        assert_true(util::is_in_range_loop(1, 0, 2, false, false)); // NOLINT
        assert_true(util::is_in_range_loop(3, 2, 0, false, false)); // NOLINT
        assert_true(!util::is_in_range_loop(1, 2, 0, true, true)); // NOLINT
        assert_true(!util::is_in_range_loop(1, 2, 3, true, true)); // NOLINT

        return 0;
    });
}