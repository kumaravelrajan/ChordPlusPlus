#include "entry.h"
#include <spdlog/fmt/fmt.h>

using entry::Command;
using entry::Entry;

Entry::Entry(const config::Configuration &conf)
{
    uint16_t dht_port = conf.p2p_port;
    uint16_t api_port = conf.api_port;

    for (size_t i = 0; i <= conf.extra_debug_nodes; i++) {
        {
            vListOfNodeInformationObj.push_back(std::make_shared<NodeInformation>("127.0.0.1", dht_port));

            // Set bootstrap node details parsed from config file
            vListOfNodeInformationObj[i]->setBootstrapNodeAddress(
                NodeInformation::Node(conf.bootstrapNode_address, conf.bootstrapNode_port)
            );

            // The constructor of Dht starts mainLoop asynchronously.
            vListOfDhtNodes.push_back(std::make_unique<dht::Dht>(vListOfNodeInformationObj[i]));

            vListOfDhtNodes[i]->setApi(std::make_unique<api::Api>(api::Options{
                .port= api_port,
            }));

            std::cout << "=================================================================================="
                      << std::endl;
            std::cout << fmt::format(
                ""
                "1. Node number   : {}\n"
                "2. Node port     : {}\n"
                "3. Node API port : {}",
                i, dht_port, api_port
            ) << std::endl;
            std::cout << "=================================================================================="
                      << std::endl;

            ++dht_port;
            ++api_port;
        }
    }
}

Entry::~Entry()
{
    std::cout << "[ENTRY] exiting..." << std::endl;
    vListOfDhtNodes.clear();
    vListOfNodeInformationObj.clear();
    std::cout << "[ENTRY] Dht Stopped." << std::endl;
}

int Entry::mainLoop()
{
    std::string line;

    while (true) {
        std::cout << "$ ";
        std::getline(std::cin, line);
        std::istringstream iss{line};
        std::vector<std::string> tokens{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
        if (tokens.empty()) continue;

        std::string cmd = tokens.front();
        tokens.erase(tokens.begin());

        if (Entry::commands.contains(cmd)) {
            const auto &command = Entry::commands.at(cmd);
            command.execute(tokens, std::cout);
        } else {
            std::cout << "Command \"" << cmd << "\" not found!" << std::endl;
        }
        std::cout << std::endl;

        if (line == "exit") break;
    }

    return 0;
}

const std::unordered_map<std::string, Command> Entry::commands{ // NOLINT
    {
        "help",
        {
            .brief="Print help for a command",
            .usage="Usage:\n"
                   "help [COMMAND]",
            .execute=[](const std::vector<std::string> &args, std::ostream &os) {
                if (args.empty()) {
                    os << "Commands:\n";
                    for (const auto &item : Entry::commands) {
                        os << fmt::format("{:<12} : {}", item.first, item.second.brief) << std::endl;
                    }
                } else {
                    if (Entry::commands.contains(args[0])) {
                        const auto &cmd = Entry::commands.at(args[0]);
                        os << cmd.brief << std::endl << std::endl << cmd.usage << std::endl;
                    } else {
                        os << "Command \"" << args[0] << "\" not found!" << std::endl;
                    }
                }
            }
        }
    },
    {
        "exit",
        {
            .brief="Exit the program",
            .usage="Usage:\n"
                   "exit",
            .execute=[](const std::vector<std::string> &, std::ostream &os) {
                os << "Shutting down...";
            }
        }
    }
};