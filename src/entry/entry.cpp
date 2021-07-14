#include "entry.h"
#include <spdlog/fmt/fmt.h>

using namespace std::literals;
using entry::Command;
using entry::Entry;

Entry::Entry(const config::Configuration &conf) : Entry()
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
    // TODO: Maybe make the streams configurable
    std::istream &is = std::cin;
    std::ostream &os = std::cout;
    // std::ostream &err = std::cerr;
    std::ostream &err = std::cout;

    std::string line;

    while (true) {
        std::cout << "$ ";
        std::getline(is, line);
        std::istringstream iss{line};
        std::vector<std::string> tokens{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
        if (tokens.empty()) continue;

        execute(tokens, os, err);
        std::cout << std::endl;

        if (line == "exit") break;
    }

    return 0;
}

void Entry::execute(std::vector<std::string> args, std::ostream &os, std::ostream &err)
{
    std::string cmd = args.front();
    args.erase(args.begin());

    if (Entry::commands.contains(cmd)) {
        const auto &command = Entry::commands.at(cmd);
        try {
            command.execute(args, os, err);
        } catch (const std::exception &e) {
            err << e.what() << std::endl;
        }
    } else {
        if (auto pos = cmd.find('.'); pos != std::string::npos)
            cmd.replace(pos, 1, " ");
        err << "Command \"" << cmd << "\" not found!" << std::endl;
    }
}

Entry::Entry() : commands{
    {
        "help",
        {
            .brief="Print help for a command",
            .usage="help [COMMAND]",
            .execute=
            [this](const std::vector<std::string> &args, std::ostream &os, std::ostream &) {
                if (args.empty()) {
                    os << "Commands:\n";
                    for (const auto &item : commands) {
                        if (item.first.find(".") != std::string::npos) continue;
                        os << fmt::format("{:<12} : {}", item.first, item.second.brief) << std::endl;
                    }
                } else {
                    std::string str = util::join(args, ".");
                    if (commands.contains(str)) {
                        const auto &cmd = commands.at(str);
                        os << cmd.brief << "\n\nUsage:\n" << cmd.usage << std::endl;
                    } else {
                        throw std::invalid_argument("Command \"" + str + "\" not found!");
                    }
                }
            }
        }
    },
    {
        "exit",
        {
            .brief="Exit the program",
            .usage="exit",
            .execute=
            [](const std::vector<std::string> &, std::ostream &os, std::ostream &) {
                os << "Shutting down..." << std::endl;
            }
        }
    },
    {
        "show",
        {
            .brief="Show data depending on arguments",
            .usage="show WHAT [ARGS...]",
            .execute=
            [this](const std::vector<std::string> &args, std::ostream &os, std::ostream &err) {
                if (args.empty())
                    throw std::invalid_argument("Argument required: WHAT");
                std::string what = util::to_lower(args[0]);
                std::vector<std::string> new_args{args.begin(), args.end()};
                new_args[0] = "show." + what;
                execute(new_args, os, err);
            }
        }
    },


    // Show-Commands
    {
        "show.nodes",
        {
            .brief="List all Nodes",
            .usage="show nodes",
            .execute=
            [this](const std::vector<std::string> &args, std::ostream &os, std::ostream &) {
                for (size_t i = 0; i < vListOfNodeInformationObj.size(); ++i) {
                    os << fmt::format(
                        "[{:>03}] : {}:{:>4}",
                        i, vListOfNodeInformationObj[i]->getIp(), vListOfNodeInformationObj[i]->getPort()
                    ) << std::endl;
                }
                os << "show nodes" << std::endl;
            }
        }
    }
} {}
