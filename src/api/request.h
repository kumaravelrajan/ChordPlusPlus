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
#include "request_data.h"

namespace API
{
    class Request
    {
    private:
        struct MessageHeader
        {
#pragma pack(push, 2)
            struct MessageHeaderRaw
            {
                uint16_t size, msg_type;
            };
#pragma pack(pop)

            uint16_t size, msg_type;

            MessageHeader(const MessageHeaderRaw &raw) // NOLINT(google-explicit-constructor)
            {
                size = util::swapBytes16(raw.size);
                msg_type = util::swapBytes16(raw.msg_type);
            }
        };

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

        template<class T, std::enable_if_t<std::is_convertible_v<std::remove_cvref_t<T>, std::vector<std::byte>>, int> = 0>
        explicit Request(T &&bytes):
            m_rawBytes(std::forward<T>(bytes))
        {
            if (m_rawBytes.size() < sizeof(MessageHeader::MessageHeaderRaw))
                throw bad_buffer_size("buffer too small for header");

            MessageHeader header = *reinterpret_cast<MessageHeader::MessageHeaderRaw *>(&m_rawBytes[0]);

            if (m_rawBytes.size() < header.size)
                throw bad_buffer_size("buffer smaller than specified in header");

            if (header.msg_type == util::constants::DHT_PUT) {
                m_decodedData = std::make_unique<Request_DHT_PUT>(m_rawBytes, sizeof(MessageHeader::MessageHeaderRaw), header.size);
            } else if (header.msg_type == util::constants::DHT_GET) {
                m_decodedData = std::make_unique<Request_DHT_GET>(m_rawBytes, sizeof(MessageHeader::MessageHeaderRaw), header.size);
            } else {
                throw bad_request("message type incorrect: " + std::to_string(header.msg_type));
            }
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
