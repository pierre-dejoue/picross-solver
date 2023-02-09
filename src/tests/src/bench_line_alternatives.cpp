#include <catch2/benchmark/catch_benchmark_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <picross/picross.h>
#include <utils/test_helpers.h>
#include <utils/text_io.h>

#include "binomial.h"
#include "line.h"
#include "line_alternatives.h"
#include "line_constraint.h"
#include "line_span.h"

namespace picross
{
namespace
{
    BinomialCoefficients::Cache& get_binomial()
    {
        static BinomialCoefficients::Cache binomial;
        return binomial;
    }

    LineAlternatives::Reduction full_reduction(const LineConstraint& constraint, const Line& known_tiles)
    {
        return LineAlternatives(constraint, known_tiles, get_binomial()).full_reduction();
    }

    LineAlternatives::Reduction partial_reduction(const LineConstraint& constraint, const Line& known_tiles, unsigned int nb_constraints)
    {
        return LineAlternatives(constraint, known_tiles, get_binomial()).partial_reduction(nb_constraints);
    }

    LineAlternatives::Reduction linear_reduction(const LineConstraint& constraint, const Line& known_tiles)
    {
        return LineAlternatives(constraint, known_tiles, get_binomial()).linear_reduction();
    }
}

#if 0
TEST_CASE("Bench full reduction", "[line_alternatives]")
{
    // Example from webpbn-03528-only-one.non
    {
        const LineConstraint constraint(Line::COL, { 3, 2, 4, 3, 1, 1, 2, 17, 2 });
        const auto known_tiles    = build_line_from("..????????????????????#?????????????????????????????????????", Line::COL, 17);
        const auto expected_delta = build_line_from("????????????????????????????????????????##??????????????????", Line::COL, 17);
        LineAlternatives::Reduction reduction;
        bool bench_run = false;
        BENCHMARK("COL 17") {
            reduction = full_reduction(constraint, known_tiles);
            return bench_run = true;
        };
        if (bench_run)
        {
            CHECK((reduction.reduced_line - known_tiles) == expected_delta);
            CHECK(reduction.nb_alternatives == 618206);
            CHECK(reduction.is_fully_reduced);
        }
    }
    // Example from tiger.non
    {
        const LineConstraint constraint(Line::ROW, { 2, 2, 2, 2, 3, 4, 2, 2, 3 });
        const auto known_tiles    = build_line_from("???????????...???????.?????????????.?..?.?????????????????#??????????????..", Line::ROW, 45);
        const auto expected_delta = build_line_from("????????????????????????????????????.??.???????????????????????????????????", Line::ROW, 45);
        LineAlternatives::Reduction reduction;
        bool bench_run = false;
        BENCHMARK("ROW 45") {
            reduction = full_reduction(constraint, known_tiles);
            return bench_run = true;
        };
        if (bench_run)
        {
            CHECK((reduction.reduced_line - known_tiles) == expected_delta);
            CHECK(reduction.nb_alternatives == 8714484);
            CHECK(reduction.is_fully_reduced);
        }
    }
    // Example from tiger.non
    {
        const LineConstraint constraint(Line::ROW, { 1, 2, 2, 1, 12, 1, 2, 2 });
        const auto known_tiles    = build_line_from("???????????...???????..???????????###??###???????????????????????????????..", Line::ROW, 46);
        const auto expected_delta = build_line_from("?????????????????????????????????????##????????????????????????????????????", Line::ROW, 46);
        LineAlternatives::Reduction reduction;
        bool bench_run = false;
        BENCHMARK("ROW 46") {
            reduction = full_reduction(constraint, known_tiles);
            return bench_run = true;
        };
        if (bench_run)
        {
            CHECK((reduction.reduced_line - known_tiles) == expected_delta);
            CHECK(reduction.nb_alternatives == 49441874);
            CHECK(reduction.is_fully_reduced);
        }
    }
    // Example from tiger.non
    {
        const LineConstraint constraint(Line::ROW, { 2, 2, 3, 3, 2, 3, 2, 2 });
        const auto known_tiles    = build_line_from("???????????...???????...???????????#....##???????????????????????????????..", Line::ROW, 48);
        const auto expected_delta = build_line_from("??????????????????????????????????#????????????????????????????????????????", Line::ROW, 48);
        LineAlternatives::Reduction reduction;
        bool bench_run = false;
        BENCHMARK("ROW 48") {
            reduction = full_reduction(constraint, known_tiles);
            return bench_run = true;
        };
        if (bench_run)
        {
            CHECK((reduction.reduced_line - known_tiles) == expected_delta);
            CHECK(reduction.nb_alternatives == 3873724);
            CHECK(reduction.is_fully_reduced);
        }
    }
}
#endif

TEST_CASE("Bench linear vs full reduction", "[line_alternatives]")
{
    const LineConstraint constraint(Line::ROW, { 3, 3, 3 });
    const auto min_line_length = constraint.min_line_size();
    CHECK(min_line_length == 11);
    for (unsigned int extra_zeros = 2; extra_zeros <= 14; extra_zeros += 2)
    {
        const unsigned int nb_alt = ((extra_zeros + 1) * (extra_zeros + 2) * (extra_zeros + 3)) / 6;
        const auto known_tiles    = build_line_from('?', min_line_length + extra_zeros, Line::ROW, 0);
        {
            LineAlternatives::Reduction reduction;
            bool bench_run = false;
            std::string bench_name = "Threes " + std::to_string(nb_alt) + " linear";
            BENCHMARK(std::move(bench_name)) {
                reduction = linear_reduction(constraint, known_tiles);
                return bench_run = true;
            };
            if (bench_run)
            {
                CHECK(reduction.nb_alternatives == nb_alt);
                CHECK(reduction.is_fully_reduced == false);
            }
        }
        {
            LineAlternatives::Reduction reduction;
            bool bench_run = false;
            std::string bench_name = "Threes " + std::to_string(nb_alt) + " full";
            BENCHMARK(std::move(bench_name)) {
                reduction = full_reduction(constraint, known_tiles);
                return bench_run = true;
            };
            if (bench_run)
            {
                CHECK(reduction.nb_alternatives == nb_alt);
                CHECK(reduction.is_fully_reduced);
            }
        }
    }
}

} // namespace picross
