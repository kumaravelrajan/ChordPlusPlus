#ifndef DHT_API_REQUEST_H
#define DHT_API_REQUEST_H

#include <iostream>
#include <cstddef>
#include <vector>
#include <memory>
#include <type_traits>
#include <stdexcept>
#include "request_data.h"

namespace API
{
    class Request
    {
    private:
        struct MessageHeader
        {
#pragma pack(push, 2)
            struct _MessageHeaderRaw
            {
                uint16_t size, msg_type;
            };
#pragma pack(pop)

            uint16_t size, msg_type;

            MessageHeader(const _MessageHeaderRaw &raw) // NOLINT(google-explicit-constructor)
            {
                size = (raw.size);
                msg_type = (raw.msg_type);
            }
        };

    public:
        class bad_buffer_size: public std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };

        // Constructors

        template<class T, std::enable_if_t<std::is_convertible_v<std::remove_cvref_t<T>, std::vector<std::byte>>, int> = 0>
        explicit Request(T &&bytes):
            m_rawBytes(std::forward<T>(bytes))
        {
            if (m_rawBytes.size() < sizeof(MessageHeader::_MessageHeaderRaw))
                throw bad_buffer_size("buffer too small");

            MessageHeader header = *reinterpret_cast<MessageHeader::_MessageHeaderRaw *>(&m_rawBytes[0]);
            std::cout << "Size: " << header.size << std::endl;
        }

        Request(Request &&other) noexcept;
        Request(const Request &other) = delete;

        Request &operator=(Request &&other) noexcept;
        Request &operator=(const Request &other) = delete;

        void sendReply(const std::vector<std::byte> &bytes);

        template<class T, std::enable_if_t<std::is_base_of_v<RequestData, std::remove_cv_t<T>>, int> = 0>
        std::remove_cv_t<T> *getData()
        {
            return dynamic_cast<typename std::remove_pointer<T>::type *>(m_decodedData.get());
        }

    private:
        std::vector<std::byte> m_rawBytes;
        std::unique_ptr<RequestData> m_decodedData;
    };
} // namespace API

#endif //DHT_API_REQUEST_H
