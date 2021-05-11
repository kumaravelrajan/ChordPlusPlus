#ifndef DHT_TEST_ASSERT_H
#define DHT_TEST_ASSERT_H

#include <iostream>
#include <string>
#include <optional>
#include <functional>
#include <exception>

static void fail(const std::optional<std::string> &message = {})
{
    throw std::exception((message.value_or("")).c_str());
}

void assert_true(bool expression, const std::optional<std::string> &message = {})
{
    if (!expression) fail(message);
}

void assert_false(bool expression, const std::optional<std::string> &message = {})
{
    assert_true(!expression, message);
}

template<class T>
void assert_not_null(T* ptr, const std::optional<std::string> &message = {})
{
    assert_true(ptr != nullptr, message);
}

template<class T>
void assert_null(T* ptr, const std::optional<std::string> &message = {})
{
    assert_true(ptr == nullptr, message);
}

int run_test(const std::function<int()> &test)
{
    try {
        return test();
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}

#endif //DHT_TEST_ASSERT_H
