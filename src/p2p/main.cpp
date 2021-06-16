#include <iostream>
#include <p2p.h>

int main(int argc, const char *argv[])
{
    std::cout << "Hello, World!" << std::endl;

    size_t t;
    std::cout << "[0]: Client\n[1]: Server" << std::endl;
    std::cin >> t;
    if (t > 1) return 1;

    if (t) {
        p2p::testServer();
    } else {
        p2p::testClient();
    }

    return 0;
}