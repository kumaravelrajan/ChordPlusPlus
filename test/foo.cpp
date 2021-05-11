#include "assertions.h"

int main()
{
    return run_test([]() {
        assert_true(true, "Message 1");
        assert_false(false, "Message 2");

        return 0;
    });
}