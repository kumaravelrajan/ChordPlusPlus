#include <iostream>
#include <chrono>
#include "api.h"

int main()
{
    using namespace std::chrono_literals;
    using namespace API;

    std::cout << "This is the main method for testing api!" << std::endl;
    {
        Api api{};
        std::this_thread::sleep_for(5s);
    }
    return 0;
}