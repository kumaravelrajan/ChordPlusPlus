#include <iostream>
#include "api.h"
#include <cxxopts.hpp>
#include <INIReader.h>

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
        // Define long and short options for cmd arguments
        const struct option longopts[] =
            {
                {"config", required_argument, 0, 'c'},
                {0,0,0,0}
            };

        int option = 0, index;
        while((option = getopt_long_only(argc, argv, "c:", longopts, &index)) != -1)
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
    std::string sConfigFilePath(ConfigFilePath);
    INIReader reader(sConfigFilePath);

    if(reader.ParseError() != 0) {
        cout << "Can't load " << ConfigFilePath <<endl;
        return -1;
    }

    cout << "api_address = " << reader.Get("DHT", "api_address", "-1") <<endl <<"p2p_address = " <<reader.Get("DHT", "p2p_address", "-1");
    return 0;
}

void PrintCmdUsage()
{
    cout <<"Error! Wrong option.\nUsage-> \nConfiguration file path: --config <Path to config file> | -c <Path to config file>";
}