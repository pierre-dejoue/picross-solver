// Copyright (c) 2023 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#include <catch_amalgamated.hpp>

#include <stdutils/span.h>

#include <array>
#include <type_traits>
#include <vector>

TEST_CASE("Trivial dynamic extent span<T>", "[span]")
{
    stdutils::Span<int> span;

    CHECK(span.size() == 0);
    CHECK(span.empty() == true);
    CHECK(span.data() == nullptr);
}

TEST_CASE("Dynamic extent span<T> to span<const T>", "[span]")
{
    std::vector<int> test_vect { 0, 2, 3, 4 };

    auto span = stdutils::Span<int>(test_vect.data(), test_vect.size());
    span[0] = 1;
    CHECK(test_vect[0] == 1);
    auto const_span = stdutils::Span<const int>(span);
    CHECK(const_span[0] == 1);
    CHECK(const_span.size() == test_vect.size());
}

TEST_CASE("Dynamic extent span<T> from containers with make_span", "[span]")
{
    std::vector<int> test_vect { 0, 2, 3, 4 };

    auto span = stdutils::make_span(test_vect);
    REQUIRE(span.size() == test_vect.size());
    span[0] = 1;
    CHECK(span[0] == 1);

    auto const_span = stdutils::make_const_span(test_vect);
    static_assert(std::is_same_v<decltype(const_span)::value_type, const int>);
    REQUIRE(const_span.size() == test_vect.size());
    CHECK(const_span[0] == 1);
}

TEST_CASE("Static extent span<T> to span<const T>", "[span]")
{
    std::vector<int> test_vect { 1, 2, 3, 4 };

    auto span = stdutils::Span<int, 2>(test_vect.data() + 2);
    REQUIRE(span.size() == 2);
    span[0] = 0;
    CHECK(test_vect[2] == 0);

    auto const_span = stdutils::Span<const int, 2>(span);
    REQUIRE(const_span.size() == 2);
    CHECK(const_span[0] == 0);
    CHECK(const_span[1] == 4);
}

TEST_CASE("Static extent span<T> on arrays", "[span]")
{
    std::array<int, 4> test_arr { 1, 2, 3, 4 };
    const std::array<int, 3> test_const_arr { 5, 6, 7 };

    auto span = stdutils::Span<int, 4>(test_arr);
    REQUIRE(span.size() == 4);
    span[2] = 0;
    CHECK(span[2] == 0);
    CHECK(test_arr[2] == 0);

    auto const_span = stdutils::Span<const int, 3>(test_const_arr);
    REQUIRE(const_span.size() == 3);
    CHECK(const_span[0] == 5);
    CHECK(const_span[1] == 6);
}

TEST_CASE("Dynamic extent span<T> on arrays", "[span]")
{
    std::array<int, 4> test_arr { 0, 2, 3, 4 };

    auto span = stdutils::make_span(test_arr);
    REQUIRE(span.size() == test_arr.size());
    span[0] = 1;
    CHECK(span[0] == 1);

    auto const_span = stdutils::make_const_span(test_arr);
    static_assert(std::is_same_v<decltype(const_span)::value_type, const int>);
    REQUIRE(const_span.size() == test_arr.size());
    CHECK(const_span[0] == 1);
}
