#include "entry.h"
#include <regex>
#include "../logging/centralLogControl.h"

using namespace std::literals;
using entry::Command;
using entry::Entry;

namespace entry
{
    template<typename T = uint32_t>
    std::optional<T> parse_number(const std::string &str)
    {
        std::smatch match;
        std::regex_match(str, match,
                         std::regex(R"(((?:\+|-)?(?:\.\d+|\d+\.?\d*)))"));
        std::string result = match[1].str();
        return
            !result.empty()
            ? util::from_string<T>(result)
            : std::optional<T>{};
    }

    std::string format_node(const NodeInformation::Node &node)
    {
        return fmt::format("{}:{:>04}", node.getIp(), node.getPort());
    }

    std::string format_node(const std::optional<NodeInformation::Node> &node)
    {
        return node ? format_node(*node) : "<null>";
    }

    std::string format_argument_choice(const std::string &metavar, const std::vector<std::string> &choices)
    {
        return fmt::format("{:>8}:    {{{}}}", metavar, fmt::join(choices, ", "));
    }

    std::string insertHighlighterSection()
    {
        return "=======================================";
    }
}


Entry::Entry(const config::Configuration &conf) : Entry()
{
    m_conf = conf;
    uint16_t dht_port = conf.p2p_port;
    uint16_t api_port = conf.api_port;

    for (size_t i = 0; i <= conf.extra_debug_nodes; i++) {
        {
            SPDLOG_DEBUG(
                "\n"
                "\t\t==================================================================================\n"
                "\t\t1. Node number   : {}\n"
                "\t\t2. Node port     : {}\n"
                "\t\t3. Node API port : {}\n"
                "\t\t==================================================================================",
                i, dht_port, api_port
            );

            m_nodes.push_back(std::make_shared<NodeInformation>(conf.p2p_address, dht_port));

            // Set bootstrap node details parsed from config file
            m_nodes[i]->setBootstrapNode(
                NodeInformation::Node(conf.bootstrapNode_address, conf.bootstrapNode_port)
            );

            // The constructor of Dht starts mainLoop asynchronously.
            m_DHTs.push_back(std::make_unique<dht::Dht>(m_nodes[i]));

            m_DHTs[i]->setApi(std::make_unique<api::Api>(api::Options{
                .port= api_port,
            }));

            ++dht_port;
            ++api_port;
        }
    }
}

Entry::~Entry()
{
    SPDLOG_TRACE("[ENTRY] exiting...");

    // Asynchronously delete dht objects
    std::vector<std::future<void>> deletions(m_DHTs.size());
    std::transform(m_DHTs.begin(), m_DHTs.end(), deletions.begin(), [](std::unique_ptr<dht::Dht> &dht) {
        std::this_thread::sleep_for(50ms);
        return std::async(std::launch::async, [&dht] { dht = nullptr; });
    });
    deletions.clear(); // await deletions
    m_DHTs.clear();
    m_nodes.clear();
    SPDLOG_TRACE("[ENTRY] Dht Stopped.");
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
        std::vector<std::string> tokens{};
        if (!isRepeatSet) {
            if (!std::getline(is, line))
                break;
            if (!isatty(fileno(stdin)))
                os << line << std::endl;
            std::regex re{R"(\[[^\]]*\]|[^ \n\r\t\[\]]+)"};
            std::transform(
                std::sregex_iterator{line.begin(), line.end(), re}, std::sregex_iterator{},
                std::back_inserter(tokens), [](const std::smatch &match) { return match.str(); });
            if (tokens.empty()) continue;

            if (tokens[0] != "repeat") {
                m_lastEnteredCommand = tokens;
            }
        } else {
            std::string toPrint = fmt::format("{}\n", fmt::join(m_lastEnteredCommand, " "));
            std::cout << toPrint;
            if (!m_lastEnteredCommand.empty()) {
                tokens = m_lastEnteredCommand;
            } else {
                /* repeat entered as the first comment. */
                fmt::print("repeat cannot be the first command. Enter any other command.\n");
                isRepeatSet = false;
                continue;
            }
        }

        isRepeatSet = false;

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

    if (Entry::m_commands.contains(cmd)) {
        const auto &command = Entry::m_commands.at(cmd);
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

void Entry::addNodeDynamicallyToNetwork(std::optional<uint16_t> portParam, std::ostream &os)
{
    uint16_t Port = portParam ? *portParam : (m_nodes.back()->getPort() + 1);

    m_nodes.push_back(std::make_shared<NodeInformation>(m_conf.p2p_address, Port));

    // Set bootstrap node details parsed from config file
    m_nodes.back()->setBootstrapNode(
        NodeInformation::Node(m_conf.bootstrapNode_address, m_conf.bootstrapNode_port)
    );

    // The constructor of Dht starts mainLoop asynchronously.
    m_DHTs.push_back(std::make_unique<dht::Dht>(m_nodes.back()));

    m_DHTs.back()->setApi(std::make_unique<api::Api>(api::Options{
        .port= static_cast<uint16_t>(m_nodes.back()->getPort() + static_cast<uint16_t>(1000)),
    }));

    os
        << "Details of new node = \n"
        << "1. P2P address - " << m_nodes.back()->getIp() << ":" << m_nodes.back()->getPort() << "\n"
        << "2. API port - " << m_nodes.back()->getPort() + static_cast<uint16_t>(1000) << "\n";
}

Entry::Entry() : m_commands{
    {
        "help",
        {
            .brief= "Print help for a command",
            .usage= "help [COMMAND]",
            .execute=
            [this](const std::vector<std::string> &args, std::ostream &os, std::ostream &) {
                if (args.empty()) {
                    os << "Commands:\n";
                    for (const auto &item : m_commands) {
                        if (item.first.find(":") != std::string::npos) continue;
                        os << fmt::format("{:<12} : {}", item.first, item.second.brief) << std::endl;
                    }
                } else {
                    std::string str = util::join(args, ":");
                    if (m_commands.contains(str)) {
                        const auto &cmd = m_commands.at(str);
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
            .brief= "Exit the program",
            .usage= "exit",
            .execute=
            [](const std::vector<std::string> &args, std::ostream &os, std::ostream &) {
                if (!args.empty())
                    throw std::invalid_argument("No arguments expected!");
                os << "Shutting down..." << std::endl;
            }
        }
    },
    {
        "sleep",
        {
            .brief= "Wait some time before accepting new input",
            .usage= "sleep <SECONDS>",
            .execute=
            [](const std::vector<std::string> &args, std::ostream &, std::ostream &) {
                auto seconds = args.empty() ? std::optional<float>{} : parse_number<float>(args[0]);
                if (!seconds)
                    throw std::invalid_argument("SECONDS required!");
                if (*seconds < 0)
                    throw std::invalid_argument(fmt::format("Invalid interval: {}", *seconds));
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<size_t>(*seconds * 1000)));
            }
        }
    },
    {
        "repeat",
        {
            .brief= "Exit the program",
            .usage= "exit",
            .execute=
            [this](const std::vector<std::string> &, std::ostream &, std::ostream &) {
                isRepeatSet = true;
            }
        }
    },
    {
        "add",
        {
            .brief= "Add node to the chord ring",
            .usage= "add [PORT]",
            .execute=
            [this](const std::vector<std::string> &args, std::ostream &os, std::ostream &) {
                bool isUserPortAvailable = true;
                if (!args.empty()) {
                    auto port = parse_number<uint16_t>(args[0]);
                    if (port) {
                        addNodeDynamicallyToNetwork(*port, os);
                    } else {
                        addNodeDynamicallyToNetwork();
                    }
                } else {
                    addNodeDynamicallyToNetwork();
                }
            }
        }
    },
    {
        "remove",
        {
            .brief= "Remove node(s) from the chord ring",
            .usage= "remove <INDEX> [COUNT]",
            .execute=
            [this](const std::vector<std::string> &args, std::ostream &, std::ostream &) {
                std::optional<uint32_t> index{};
                if (!args.empty())
                    index = parse_number(args[0]);
                if (!index)
                    throw std::invalid_argument("INDEX required!");
                if (index >= m_nodes.size())
                    throw std::invalid_argument(fmt::format("Index [{}] out of bounds!", *index));
                uint32_t count = 1;
                if (args.size() > 1) {
                    auto c = parse_number(args[1]);
                    if (c) {
                        if (*index + *c > m_nodes.size())
                            throw std::invalid_argument(fmt::format("Count [{}] out of bounds!", *c));
                        count = *c;
                    }
                }

                std::vector<std::future<void>> deletions{};
                std::transform(
                    m_DHTs.begin() + *index, m_DHTs.begin() + *index + count, std::back_inserter(deletions),
                    [](std::unique_ptr<dht::Dht> &dht) {
                        std::this_thread::sleep_for(50ms);
                        return std::async(std::launch::async, [&dht] { dht = nullptr; });
                    }
                );
                deletions.clear(); // await deletions

                m_DHTs.erase(m_DHTs.begin() + *index, m_DHTs.begin() + *index + count);
                m_nodes.erase(m_nodes.begin() + *index, m_nodes.begin() + *index + count);
            }
        }
    },
    {
        "show",
        {
            .brief= "Show data depending on arguments",
            .usage= "show <WHAT> [ARGS...]\n\n" +
                    format_argument_choice("WHAT", {"nodes", "data", "fingers"}),
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
            .brief= "List all Nodes, or information of one Node",
            .usage= "show nodes [INDEX]",
            .execute=
            [this](const std::vector<std::string> &args, std::ostream &os, std::ostream &err) {
                std::optional<uint32_t> index{};
                if (!args.empty()) {
                    if (args[0] == "data") {
                        std::string what = util::to_lower(args[0]);
                        std::vector<std::string> new_args{args.begin(), args.end()};
                        new_args[0] = "show:nodes:" + what;
                        execute(new_args, os, err);
                        return;
                    } else {
                        index = parse_number(args[0]);
                    }
                }

                if (index) {
                    if (index >= m_nodes.size())
                        throw std::invalid_argument("Index [" + util::to_string(*index) + "] out of bounds!");
                    auto &node = *m_nodes[*index];

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
                } else {
                    for (size_t i = 0; i < m_nodes.size(); ++i) {
                        os << fmt::format(
                            "[{:>03}] : {}",
                            i, format_node(m_nodes[i]->getNode())
                        ) << std::endl;
                    }
                }
            }
        }
    },
    {
        "show:data",
        {
            .brief="List all data items contained in Node",
            .usage="show nodes [INDEX]",
            .execute=
            [this](const std::vector<std::string> &args, std::ostream &os, std::ostream &err) {
                if (!args.empty()) {
                    auto index = parse_number<uint32_t>(args[0]);
                    if (index) {
                        dataItem_type dataInNode = m_nodes[*index]->getAllDataInNode();

                        /* Display node details. */
                        std::vector<std::string> new_Args = {"show", "nodes", std::to_string(*index)};
                        execute(new_Args, os, err);

                        /* Highlighting the data keys */
                        os << fmt::format("\n{}\n", insertHighlighterSection());

                        if (!dataInNode.empty()) {
                            os << "Data items in node:\n";
                            int i = 1;
                            for (const auto &s : dataInNode) {
                                // Hashing received key to convert it into length of 20 bytes
                                std::string sKey{s.first.begin(), s.first.end()};
                                NodeInformation::id_type finalHashedKey = NodeInformation::hash_sha1(sKey);
                                os << fmt::format("{}. key = {}\n", i, util::hexdump(finalHashedKey, 20, false, false));
                                ++i;
                            }
                            os << fmt::format("{}\n", insertHighlighterSection());
                        } else {
                            os << fmt::format("No data items stored in node {}\n", *index);
                            os << fmt::format("{}\n", insertHighlighterSection());
                        }
                    }
                } else {
                    os << "INDEX required!";
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
                    index = parse_number(args[0]);
                if (!index)
                    throw std::invalid_argument("INDEX required!");
                if (index >= m_nodes.size())
                    throw std::invalid_argument(fmt::format("Index [{}] out of bounds!", *index));

                const auto &node = *m_nodes[*index];

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
                        ) << std::endl;
                        printed_star = false;
                    }
                    last_finger = &finger;
                }
            }
        }
    }
} {}
