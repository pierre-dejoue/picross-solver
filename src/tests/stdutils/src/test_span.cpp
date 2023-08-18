#include <catch_amalgamated.hpp>

#include <stdutils/span.h>

#include <vector>

TEST_CASE("Dynamic extent span<T> to span<const T>", "[span]")
{
    std::vector<int> test_vect { 0, 2, 3, 4 };

    auto span = stdutils::span<int>(test_vect.data(), test_vect.size());
    span[0] = 1;
    CHECK(test_vect[0] == 1);

    auto const_span = stdutils::span<const int>(span);
    CHECK(const_span[0] == 1);
    CHECK(const_span.size() == test_vect.size());
}

TEST_CASE("Static extent span<T> to span<const T>", "[span]")
{
    std::vector<int> test_vect { 1, 2, 3, 4 };

    auto span = stdutils::span<int, 2>(test_vect.data() + 2);
    REQUIRE(span.size() == 2);
    span[0] = 0;
    CHECK(test_vect[2] == 0);

    auto const_span = stdutils::span<const int, 2>(span);
    REQUIRE(const_span.size() == 2);
    CHECK(const_span[0] == 0);
    CHECK(const_span[1] == 4);
}
