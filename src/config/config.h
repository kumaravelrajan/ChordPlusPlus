#ifndef DHT_CONFIG_H
#define DHT_CONFIG_H

#include <string>
#include <cstdint>
#include <optional>

namespace config
{
#define DEFAULT_DIFFICULTY 1

    struct Configuration
    {
        std::string private_key_path{"./hostkey.pem"};

        std::string certificate_path{"./hostcert.crt"};
        bool use_tls{false};
        std::string p2p_address{"127.0.0.1"};
        uint16_t p2p_port{6002};
        std::string api_address{"127.0.0.1"};
        uint16_t api_port{7002};
        std::string bootstrapNode_address{"127.0.0.1"};
        uint16_t bootstrapNode_port{6002};
        uint64_t node_amount{1};
        std::optional<std::string> startup_script{};
        static uint8_t PoW_Difficulty;
    };

    Configuration parseConfigFile(const std::string &path);
}

#endif //DHT_CONFIG_H
