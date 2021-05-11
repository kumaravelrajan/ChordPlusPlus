#ifndef DHT_API_REQUEST_H
#define DHT_API_REQUEST_H

#include <cstddef>
#include <vector>
#include <memory>
#include "request_data.h"

class Request
{
public:
    explicit Request(std::vector<std::byte> bytes);
    Request(Request &&other) noexcept;
    Request(const Request &other) = delete;

    Request &operator=(Request &&other) noexcept;
    Request &operator=(const Request &other) = delete;

    void sendReply(const std::vector<std::byte> &bytes);

    template<class T>
    T *getData()
    {
        static_assert(std::is_base_of<RequestData, std::remove_pointer<T>>::value, "T must inherit RequestData!");
        return dynamic_cast<std::remove_pointer<T> *>(m_decodedData.get());
    }

private:
    std::vector<std::byte> m_rawBytes;
    std::unique_ptr<RequestData> m_decodedData;
};

#endif //DHT_API_REQUEST_H
