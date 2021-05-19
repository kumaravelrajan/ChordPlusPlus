#include <iostream>
#include "api.h"

int ParseConfigFile(char *ConfigFilePath);

int main(int argc, char* argv[])
{
    std::cout << "Hello, World!" << std::endl;

    if(argc < 2)
    {
        std::cout << "No configuration file path given. Program failed.";
        return -1;
    }
    else if(argc == 2)
    {
        // configuration file path given.
        ParseConfigFile(argv[1]);
    }

    return 0;
}

int ParseConfigFile(char *ConfigFilePath)
{

}