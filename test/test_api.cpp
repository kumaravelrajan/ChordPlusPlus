#include <iostream>
#include <cstdint>
#include "test_assert.h"
#include "api.h"

int main()
{
    return run_test([]() {
        std::cout << "Hello, World!" << std::endl;

        std::string bytesString = "\x12\x15";
        std::vector<std::byte> bytes(bytesString.size());
        std::transform(bytesString.begin(), bytesString.end(), bytes.begin(), [](char c) { return std::byte(c); });

        API::Request request(bytes);

        return 0;
    });
}