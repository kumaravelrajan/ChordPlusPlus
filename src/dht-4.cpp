#include <iostream>
#include "api.h"

int ParseConfigFile(char *ConfigFilePath);
void PrintCmdUsage();

using namespace std;

int main(int argc, char* argv[])
{
    std::cout << "Hello, World!" << std::endl;

    if(argc < 3) // "-c PathToConfigFile" not given.
    {
        std::cout << "No configuration file path given. Program failed.";
        return -1;
    }
    else if(argc > 2)
    {
        int option = 0;
        while((option = getopt(argc, argv, "c:")) != -1)
        {
            switch(option)
            {
            case 'c':
                ParseConfigFile(optarg);
                break;

            default:
                PrintCmdUsage();
                break;
            }
        }
    }
    return 0;
}

int ParseConfigFile(char *ConfigFilePath)
{

}

void PrintCmdUsage()
{
    cout <<"Error! Wrong option.\nUsage: -c <Path to config file>";
}