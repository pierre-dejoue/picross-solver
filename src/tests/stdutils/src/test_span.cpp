#include <catch_amalgamated.hpp>
#include <stdutils/span.h>

#include <vector>

TEST_CASE("span<T> to span<const T>", "[span]")
{
    std::vector<int> test_vect { 0, 2, 3, 4 };

    auto span = stdutils::span(test_vect.data(), test_vect.size());
    span[0] = 1;
    CHECK(test_vect[0] == 1);

    auto const_span = stdutils::span<const int>(span);
    CHECK(const_span[0] == 1);
    CHECK(const_span.size() == test_vect.size());
}
