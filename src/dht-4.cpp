#include <iostream>
#include <string>
#include "api.h"

int ParseConfigFile(char *ConfigFilePath);

using namespace std;

int main(int argc, char* argv[])
{
    std::cout << "Hello, World!" << std::endl;

    if(argc < 3)
    {
        std::cout << "No configuration file path given. Program failed.";
        return -1;
    }
    else if(argc == 3)
    {
        // Check if configuration file path given.
        if(strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "-C"))
        {
            ParseConfigFile(argv[2]);
        }
    }

    return 0;
}

int ParseConfigFile(char *ConfigFilePath)
{

}