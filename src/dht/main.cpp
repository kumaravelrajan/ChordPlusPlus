#include <iostream>
#include "dht.h"

int main()
{
    std::cout << "This is the main method for testing dht!" << std::endl;

    int p[2];
    if (pipe(p) < 0)
        exit(1);

    dht::writeMessage(p[1]);

    dht::printMessage(p[0]);

    return 0;
}
