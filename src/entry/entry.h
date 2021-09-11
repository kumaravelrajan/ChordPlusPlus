#ifndef DHT_ENTRY_H
#define DHT_ENTRY_H

#include <iostream>
#include <fstream>
#include <config.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <stack>
#include <NodeInformation.h>
#include <Dht.h>


namespace entry
{
    using dataItem_type = std::map<std::vector<uint8_t>, std::pair<std::vector<uint8_t>, std::chrono::system_clock::time_point>>;

    class Entry;

    struct Command
    {
        std::string brief;
        std::string usage;
        std::function<void(const std::vector<std::string> &args, std::ostream &os, std::ostream &err)> execute;
    };

    class Entry
    {
    private:
        Entry();
    public:
        explicit Entry(const config::Configuration &conf);
        ~Entry();

        int mainLoop();

        void execute(std::vector<std::string> args, std::ostream &os = std::cout, std::ostream &err = std::cerr);
        void addNode(std::optional<uint16_t> port = {}, std::ostream &os = std::cout);

    private:
        std::vector<std::unique_ptr<dht::Dht>> m_DHTs{};
        std::vector<std::shared_ptr<NodeInformation>> m_nodes{};
        const std::unordered_map<std::string, Command> m_commands;
        config::Configuration m_conf{};
        std::string m_lastEnteredCommand{};
        bool isRepeatSet{false};

        std::stack<std::unique_ptr<std::ifstream>> m_input_files{};
        std::vector<std::string> m_input_filenames{};
    };
}

#endif //DHT_ENTRY_H
