#ifndef DHT_API_REQUEST_H
#define DHT_API_REQUEST_H

#include <iostream>
#include <string>
#include <cstddef>
#include <vector>
#include <memory>
#include <type_traits>
#include <stdexcept>
#include <util.h>
#include <constants.h>
#include "message_data.h"

namespace api
{
    class Request
    {
    public:
        class bad_buffer_size: public std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };

        class bad_request: public std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };

        // Constructors

        template<class T, std::enable_if_t<std::is_convertible_v<std::remove_cvref_t<T>, std::vector<uint8_t>>, int> = 0>
        explicit Request(T &&bytes);

        Request(Request &&other) noexcept;
        Request(const Request &other) = delete;

        Request &operator=(Request &&other) noexcept;
        Request &operator=(const Request &other) = delete;

        [[nodiscard]] std::vector<uint8_t> getBytes() const;

        template<class T = MessageData, std::enable_if_t<std::is_base_of_v<MessageData, std::remove_cv_t<T>>, int> = 0>
        std::remove_cv_t<T> *getData() const
        {
            return dynamic_cast<typename std::remove_pointer<T>::type *>(m_decodedData.get());
        }

    private:
        std::vector<uint8_t> m_rawBytes;
        std::unique_ptr<MessageData> m_decodedData;
    };
} // namespace API

#endif //DHT_API_REQUEST_H
