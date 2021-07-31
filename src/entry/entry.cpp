#include "entry.h"
#include <regex>
#include <centralLogControl.h>

using namespace std::literals;
using entry::Command;
using entry::Entry;

namespace entry
{
    std::optional<uint32_t> get_index(const std::string &str)
    {
        std::smatch match;
        std::regex_match(str, match,
                         std::regex(R"(((?:\d|\.\')+)|\[((?:\d|\.\')+)\])"));
        std::string result = match[1].str() + match[2].str();
        return
            !result.empty()
            ? util::from_string<uint32_t>(result)
            : std::optional<uint32_t>{};
    }

    std::string format_node(const NodeInformation::Node &node)
    {
        return fmt::format("{}:{:>04}", node.getIp(), node.getPort());
    }

    std::string format_node(const std::optional<NodeInformation::Node> &node)
    {
        return node ? format_node(*node) : "<null>";
    }
}

Entry::Entry(const config::Configuration &conf) : Entry()
{
    uint16_t dht_port = conf.p2p_port;
    uint16_t api_port = conf.api_port;

    for (size_t i = 0; i <= conf.extra_debug_nodes; i++) {
        {
            SPDLOG_INFO(
                "\n"
                "\t\t==================================================================================\n"
                "\t\t1. Node number   : {}\n"
                "\t\t2. Node port     : {}\n"
                "\t\t3. Node API port : {}\n"
                "\t\t==================================================================================\n",
                i, dht_port, api_port
            );

            nodes.push_back(std::make_shared<NodeInformation>("127.0.0.1", dht_port));

            // Set bootstrap node details parsed from config file
            nodes[i]->setBootstrapNode(
                NodeInformation::Node(conf.bootstrapNode_address, conf.bootstrapNode_port)
            );

            // The constructor of Dht starts mainLoop asynchronously.
            DHTs.push_back(std::make_unique<dht::Dht>(nodes[i]));

            DHTs[i]->setApi(std::make_unique<api::Api>(api::Options{
                .port= api_port,
            }));

            ++dht_port;
            ++api_port;
        }
    }
}

Entry::~Entry()
{
    SPDLOG_INFO("[ENTRY] exiting...\n");
    DHTs.clear();
    nodes.clear();
    SPDLOG_INFO("[ENTRY] Dht Stopped.\n");
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
        std::regex re{R"(\[[^\]]*\]|[^ \n\r\t\[\]]+)"};
        std::vector<std::string> tokens{};
        std::transform(
            std::sregex_iterator{line.begin(), line.end(), re}, std::sregex_iterator{},
            std::back_inserter(tokens), [](const std::smatch &match) { return match.str(); });
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
        if (auto pos = cmd.find(':'); pos != std::string::npos)
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
                        if (item.first.find(":") != std::string::npos) continue;
                        os << fmt::format("{:<12} : {}", item.first, item.second.brief) << std::endl;
                    }
                } else {
                    std::string str = util::join(args, ":");
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
            [](const std::vector<std::string> &args, std::ostream &os, std::ostream &) {
                if (!args.empty())
                    throw std::invalid_argument("No arguments expected!");
                os << "Shutting down..." << std::endl;
            }
        }
    },
    {
        "show",
        {
            .brief="Show data depending on arguments",
            .usage="show <WHAT> [ARGS...]",
            .execute=
            [this](const std::vector<std::string> &args, std::ostream &os, std::ostream &err) {
                if (args.empty())
                    throw std::invalid_argument("Argument required: WHAT");
                std::string what = util::to_lower(args[0]);
                std::vector<std::string> new_args{args.begin(), args.end()};
                new_args[0] = "show:" + what;
                execute(new_args, os, err);
            }
        }
    },


    // Show-Commands
    {
        "show:nodes",
        {
            .brief="List all Nodes, or information of one Node",
            .usage="show nodes [INDEX]",
            .execute=
            [this](const std::vector<std::string> &args, std::ostream &os, std::ostream &) {
                std::optional<uint32_t> index{};
                if (!args.empty())
                    index = get_index(args[0]);

                if (index) {
                    if (index >= nodes.size()) {
                        throw std::invalid_argument("Index [" + util::to_string(*index) + "] out of bounds!");
                    }
                    auto &node = *nodes[*index];

                    os << fmt::format(
                        ""
                        "nodes[{:>03}]    : {}"            "\n"
                        "  id          : {}"               "\n"
                        "  successor   : {}"               "\n"
                        "  predecessor : {}"               "\n"
                        "  bootstrap   : {}"               "\n"
                        /* == == == == */,
                        *index,
                        format_node(node.getNode()),
                        util::hexdump(node.getId(), 20, false, false),
                        format_node(node.getSuccessor()),
                        format_node(node.getPredecessor()),
                        format_node(node.getBootstrapNode())
                    ) << std::endl;
                    os << "show nodes " << *index << std::endl;
                } else {
                    for (size_t i = 0; i < nodes.size(); ++i) {
                        os << fmt::format(
                            "[{:>03}] : {}",
                            i, format_node(nodes[i]->getNode())
                        ) << std::endl;
                    }
                }
            }
        }
    },
    {
        "show:fingers",
        {
            .brief= "Show finger table of a node",
            .usage= "show fingers <INDEX>",
            .execute=
            [this](const std::vector<std::string> &args, std::ostream &os, std::ostream &) {
                std::optional<uint32_t> index{};
                if (!args.empty())
                    index = get_index(args[0]);
                if (!index)
                    throw std::invalid_argument("INDEX required!");

                const auto &node = *nodes[*index];

                const std::optional<NodeInformation::Node> *last_finger = nullptr;
                bool printed_star = false;
                for (size_t i = 0; i < NodeInformation::key_bits; ++i) {
                    const auto &finger = node.getFinger(i);
                    const auto *next_finger = (i < NodeInformation::key_bits - 1) ? &node.getFinger(i + 1) : nullptr;
                    if (next_finger && last_finger && *last_finger == finger && *next_finger == finger) {
                        if (!printed_star) {
                            os << "*\n";
                            printed_star = true;
                        }
                    } else {
                        os << fmt::format(
                            "[{:>03}] : {}",
                            i, format_node(finger)
                        ) << '\n';
                        printed_star = false;
                    }
                    last_finger = &finger;
                }
            }
        }
    }
} {}
