#ifndef DHT_ENTRY_H
#define DHT_ENTRY_H

#include <iostream>
#include <config.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <NodeInformation.h>
#include <Dht.h>

namespace entry
{
    struct Command
    {
        std::string brief;
        std::string usage;
        std::function<void(const std::vector<std::string> &args, std::ostream &os)> execute;
    };

    class Entry
    {
    public:
        explicit Entry(const config::Configuration &conf);
        ~Entry();

        int mainLoop();

    private:
        std::vector<std::unique_ptr<dht::Dht>> vListOfDhtNodes{};
        std::vector<std::shared_ptr<NodeInformation>> vListOfNodeInformationObj{};
        static const std::unordered_map<std::string, Command> commands;
    };
}

#endif //DHT_ENTRY_H
