#include <iostream>
#include <cstdint>
#include "assertions.h"
#include "api.h"

int main()
{
    return run_test([]() {
        std::cout << "Hello, World!" << std::endl;

        using namespace std::string_literals;
        std::string bytesString =
            "\x00\x24\x28\x0b"s  // size, DHT GET
            "\x01\x02\x03\x04"s  // key
            "\x05\x06\x07\x08"s  // key
            "\x09\x0a\x0b\x0c"s  // key
            "\x0d\x0e\x0f\x10"s  // key
            "\x11\x12\x13\x14"s  // key
            "\x15\x16\x17\x18"s  // key
            "\x19\x1a\x1b\x1c"s  // key
            "\x1d\x1e\x1f\x20"s; // key
        std::vector<std::byte> bytes(bytesString.size());
        std::transform(bytesString.begin(), bytesString.end(), bytes.begin(), [](char c) { return std::byte(c); });

        API::Request request(bytes);

        assert_not_null(request.getData<API::Request_DHT_GET>(), "Data should not be null!");

        return 0;
    });
}