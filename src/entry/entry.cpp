#include "entry.h"
#include <regex>
#include "../logging/centralLogControl.h"

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

Entry::Entry() : m_commands{
    {
        "help",
        {
            .brief="Print help for a command",
            .usage="help [COMMAND]",
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
        "add",
        {
            .brief = "Add node to the chord ring",
            .usage = "add [PORT]",
            .execute=[this](const std::vector<std::string> &args, std::ostream &os, std::ostream &err) {
                bool isUserPortAvailable = true;
                if(!args.empty()){
                    uint16_t nodeP2p_Port = 0;
                    if(static_cast<uint16_t>(stoi(args[0])) <= 65535){
                        nodeP2p_Port = static_cast<uint16_t>(stoi(args[0]));
                    }
                    m_nodes.push_back(std::make_shared<NodeInformation>(m_conf.p2p_address, nodeP2p_Port));

                    // Set bootstrap node details parsed from config file
                    m_nodes.back()->setBootstrapNode(
                        NodeInformation::Node(m_conf.bootstrapNode_address, m_conf.bootstrapNode_port)
                        );

                    // The constructor of Dht starts mainLoop asynchronously.
                    m_DHTs.push_back(std::make_unique<dht::Dht>(m_nodes.back()));

                    m_DHTs.back()->setApi(std::make_unique<api::Api>(api::Options{
                        .port= m_nodes.back()->getPort() + static_cast<uint16_t>(1000)
                        }));

                    std::cout << "Details of new node = \n"
                    << "1. P2P address - " << m_nodes.back()->getIp() << ":" << m_nodes.back()->getPort() << "\n"
                    << "2. API port - " << m_conf.api_port + m_conf.extra_debug_nodes + 1 << "\n";
                }

                if(!isUserPortAvailable || args.empty()){
                    uint16_t lastPortValue = m_nodes.back()->getPort();
                    m_nodes.push_back(std::make_shared<NodeInformation>(m_conf.p2p_address, ++lastPortValue));

                    // Set bootstrap node details parsed from config file
                    m_nodes.back()->setBootstrapNode(
                        NodeInformation::Node(m_conf.bootstrapNode_address, m_conf.bootstrapNode_port)
                        );

                    // The constructor of Dht starts mainLoop asynchronously.
                    m_DHTs.push_back(std::make_unique<dht::Dht>(m_nodes.back()));

                    m_DHTs.back()->setApi(std::make_unique<api::Api>(api::Options{
                        .port= m_nodes.back()->getPort() + static_cast<uint16_t>(1000),
                        }));

                    std::cout << "Details of new node = \n"
                    << "1. P2P address - " << m_nodes.back()->getIp() << ":" << m_nodes.back()->getPort() << "\n"
                    << "2. API port - " << m_conf.api_port + m_conf.extra_debug_nodes + 1 << "\n";
                }
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
                    if (index >= m_nodes.size()) {
                        throw std::invalid_argument("Index [" + util::to_string(*index) + "] out of bounds!");
                    }
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
                    os << "show nodes " << *index << std::endl;
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
                        ) << '\n';
                        printed_star = false;
                    }
                    last_finger = &finger;
                }
            }
        }
    }
} {}
