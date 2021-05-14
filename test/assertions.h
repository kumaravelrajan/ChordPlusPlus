#ifndef DHT_ASSERTIONS_H
#define DHT_ASSERTIONS_H

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
void assert_not_null(T *ptr, const std::optional<std::string> &message = {})
{
    assert_true(ptr != nullptr, message);
}

template<class T>
void assert_null(T *ptr, const std::optional<std::string> &message = {})
{
    assert_true(ptr == nullptr, message);
}

template<class T_EXPECTED, class T_ACTUAL>
void assert_equal(const T_EXPECTED &expected, const T_ACTUAL &actual, const std::optional<std::string> &message = {})
{
    assert_true(
        expected == actual,
        "Expected: [" +
            std::to_string(expected) +
            "], actual: [" +
            std::to_string(actual) +
            "]" + (message ? (" (" + (*message) + ")") : ""));
}

int run_test(const std::string &name, const std::function<int()> &test)
{
    std::cout << "\nRunning Test [" << name << "]" << std::endl;
    try {
        return test();
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "unknown exception" << std::endl;
        return -1;
    }
}

#endif //DHT_ASSERTIONS_H
