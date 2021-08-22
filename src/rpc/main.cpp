#include <iostream>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/bundled/color.h>

#include "rpc.h"

int main()
{
    std::cout
        << fmt::format(
            fg(fmt::color::aquamarine) | fmt::emphasis::bold,
            "{1}, {0}!",
            "World", "Hello")
        << std::endl;
    return 0;
}