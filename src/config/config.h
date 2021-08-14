#ifndef DHT_CONFIG_H
#define DHT_CONFIG_H

#include <string>
#include <cstdint>

namespace config
{
    struct Configuration
    {
        std::string p2p_address{"127.0.0.1"};
        uint16_t p2p_port{6002};
        std::string api_address{"127.0.0.1"};
        uint16_t api_port{7002};
        std::string bootstrapNode_address{"127.0.0.1"};
        uint16_t bootstrapNode_port{6002};
        uint64_t extra_debug_nodes{0};
    };

    Configuration parseConfigFile(const std::string &path);
}

#endif //DHT_CONFIG_H
