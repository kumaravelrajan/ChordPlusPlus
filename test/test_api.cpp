#include <iostream>
#include <cstdint>
#include "assertions.h"
#include <api.h>
#include <util.h>
#include <constants.h>

int main()
{
    using namespace std::string_literals;
    return //
        run_test("API DECODE GET", []() {
            std::string byteString =
                "\x00\x24\x02\x8b"s  // size, DHT GET
                "\x01\x02\x03\x04"s  // key
                "\x05\x06\x07\x08"s  // key
                "\x09\x0a\x0b\x0c"s  // key
                "\x0d\x0e\x0f\x10"s  // key
                "\x11\x12\x13\x14"s  // key
                "\x15\x16\x17\x18"s  // key
                "\x19\x1a\x1b\x1c"s  // key
                "\x1d\x1e\x1f\x20"s; // key
            auto bytes = util::convertToBytes(byteString);

            API::Request request(bytes);

            auto *request_data = request.getData<API::Message_KEY>();
            assert_null(request.getData<API::Message_KEY_VALUE>());
            assert_not_null(request_data, "Data should not be null!");

            std::cout << "Key:" << std::endl;
            util::hexdump(request_data->key, 4);

            return 0;
        }) ||
        run_test("API DECODE PUT", []() {
            std::vector<int> a { 1 };
            std::vector<int> b(a.begin(), a.begin());

            std::string byteString =
                "\x00\x4b\x02\x8a"s       // size, DHT PUT
                "\x01\x02\x03\x04"s       // key
                "\x05\x06\x07\x08"s       // key
                "\x09\x0a\x0b\x0c"s       // key
                "\x0d\x0e\x0f\x10"s       // key
                "\x11\x12\x13\x14"s       // key
                "\x15\x16\x17\x18"s       // key
                "\x19\x1a\x1b\x1c"s       // key
                "\x1d\x1e\x1f\x20"s       // key
                "This is the value "s     // value
                "that is to be stored."s; // value

            auto bytes = util::convertToBytes(byteString);

            API::Request request(bytes);

            auto *request_data = request.getData<API::Message_KEY_VALUE>();
            assert_null(request.getData<API::Message_KEY>());
            assert_not_null(request_data, "Data should not be null!");

            std::cout << "Key:" << std::endl;
            util::hexdump(request_data->key, 4);
            std::cout << std::endl;
            std::cout << "Value:" << std::endl;
            util::hexdump(request_data->value, 16);

            return 0;
        }) ||
        run_test("API ENCODE PUT", []() {
            std::string sKey =
                "\x01\x02\x03\x04"s  // key
                "\x05\x06\x07\x08"s  // key
                "\x09\x0a\x0b\x0c"s  // key
                "\x0d\x0e\x0f\x10"s  // key
                "\x0d\x0e\x0f\x10"s  // key
                "\x11\x12\x13\x14"s  // key
                "\x15\x16\x17\x18"s  // key
                "\x19\x1a\x1b\x1c"s  // key
                "\x1d\x1e\x1f\x20"s; // key
            std::string sValue =
                "This is the value, and it has an arbitrary size."s;
            auto key = util::convertToBytes(sKey);
            auto value = util::convertToBytes(sValue);

            auto sizeExpected = sizeof(API::MessageHeader::MessageHeaderRaw) + key.size() + value.size();

            API::Message_KEY_VALUE message(util::constants::DHT_PUT, key, value);

            auto header = API::MessageHeader(reinterpret_cast<API::MessageHeader::MessageHeaderRaw &>(message.m_bytes[0]));

            assert_equal(sizeExpected, static_cast<uint16_t>(header.size), "Size specified in header");
            assert_equal(util::constants::DHT_PUT, header.msg_type, "Message type specified in header");

            std::cout << "Message:" << std::endl;
            util::hexdump(message.m_bytes, 16);

            return 0;
        });
}