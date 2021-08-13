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
    class Entry;

    struct Command
    {
        std::string brief;
        std::string usage;
        std::function<void(const std::vector<std::string> &args, std::ostream &os, std::ostream &err)> execute;
    };

    class Entry
    {
    public:
        Entry();
        explicit Entry(const config::Configuration &conf);
        ~Entry();

        int mainLoop();

        void execute(std::vector<std::string> args, std::ostream &os = std::cout, std::ostream &err = std::cerr);
        void addNodeDynamicallyToNetwork(uint16_t port);

    private:
        std::vector<std::unique_ptr<dht::Dht>> m_DHTs{};
        std::vector<std::shared_ptr<NodeInformation>> m_nodes{};
        const std::unordered_map<std::string, Command> m_commands;
        config::Configuration m_conf;
    };
}

#endif //DHT_ENTRY_H
