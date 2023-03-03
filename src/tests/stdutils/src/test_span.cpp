#include <catch2/catch_test_macros.hpp>
#include <stdutils/span.h>

#include <vector>

TEST_CASE("basic behavior", "[span]")
{
    std::vector<int> test_vect { 0, 2, 3, 4 };

    auto span = stdutils::span(test_vect.data(), test_vect.size());
    span[0] = 1;

    CHECK(test_vect[0] == 1);
}
