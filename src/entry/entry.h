#ifndef DHT_ENTRY_H
#define DHT_ENTRY_H

#include <config.h>
#include <vector>
#include <memory>
#include <NodeInformation.h>
#include <Dht.h>

namespace entry
{
    class Entry
    {
    public:
        explicit Entry(const config::Configuration &conf);
        ~Entry();

        int mainLoop();

    private:
        std::vector<std::unique_ptr<dht::Dht>> vListOfDhtNodes = {};
        std::vector<std::shared_ptr<NodeInformation>> vListOfNodeInformationObj = {};
    };
}

#endif //DHT_ENTRY_H
