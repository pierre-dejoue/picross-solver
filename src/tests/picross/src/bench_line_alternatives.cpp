#include <catch_amalgamated.hpp>
#include <picross/picross.h>
#include <utils/test_helpers.h>
#include <utils/text_io.h>

#include "binomial.h"
#include "line.h"
#include "line_alternatives.h"
#include "line_constraint.h"
#include "line_span.h"

namespace picross {
namespace {
    binomial::Cache& get_binomial()
    {
        static binomial::Cache binomial;
        return binomial;
    }

    LineAlternatives::Reduction full_reduction(const LineConstraint& constraint, const Line& known_tiles, FullReductionBuffers* buffers = nullptr)
    {
        return LineAlternatives(constraint, known_tiles, get_binomial()).full_reduction(buffers);
    }

    LineAlternatives::Reduction linear_reduction(const LineConstraint& constraint, const Line& known_tiles)
    {
        return LineAlternatives(constraint, known_tiles, get_binomial()).linear_reduction();
    }
}

TEST_CASE("Bench full reduction", "[line_alternatives]")
{
    // A trivial example with a lone line and 1 segment
    {
        const LineConstraint constraint(Line::ROW, { 3 });
        const auto known_tiles    = build_line_from("??????????????????????????????????????????????????????????????????????????#?????????????????????????", Line::ROW, 0);
        const auto expected       = build_line_from("........................................................................??#??.......................", Line::ROW, 0);
        LineAlternatives::Reduction reduction;
        bool bench_run = false;
        BENCHMARK("N_100_K_1") {
            reduction = full_reduction(constraint, known_tiles);
            bench_run = true;
            return reduction;
        };
        if (bench_run)
        {
            CHECK(reduction.reduced_line == expected);
            CHECK(reduction.nb_alternatives == 3);
            CHECK(reduction.is_fully_reduced);
        }
    }
    // A trivial example with a lone line and 2 segments
    {
        const LineConstraint constraint(Line::ROW, { 3, 3 });
        const auto known_tiles    = build_line_from("??????????????????????????????????????????????????????????????????????????#?????????????????????????", Line::ROW, 0);
        const auto expected       = build_line_from("??????????????????????????????????????????????????????????????????????????#?????????????????????????", Line::ROW, 0);
        LineAlternatives::Reduction reduction;
        bool bench_run = false;
        BENCHMARK("N_100_K_2") {
            reduction = full_reduction(constraint, known_tiles);
            bench_run = true;
            return  true;
        };
        if (bench_run)
        {
            CHECK(reduction.reduced_line == expected);
            CHECK(reduction.nb_alternatives == 273);
            CHECK(reduction.is_fully_reduced);
        }
    }
    // Example from webpbn-03528-only-one.non
    {
        const LineConstraint constraint(Line::COL, { 3, 2, 4, 3, 1, 1, 2, 17, 2 });
        const auto known_tiles    = build_line_from("..????????????????????#?????????????????????????????????????", Line::COL, 17);
        const auto expected_delta = build_line_from("????????????????????????????????????????##??????????????????", Line::COL, 17);
        LineAlternatives::Reduction reduction;
        bool bench_run = false;
        BENCHMARK("COL 17") {
            reduction = full_reduction(constraint, known_tiles);
            bench_run = true;
            return reduction;
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
            bench_run = true;
            return reduction;
        };
        if (bench_run)
        {
            CHECK((reduction.reduced_line - known_tiles) == expected_delta);
            CHECK(reduction.nb_alternatives == 88714484);
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
            bench_run = true;
            return reduction;
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
            bench_run = true;
            return reduction;
        };
        if (bench_run)
        {
            CHECK((reduction.reduced_line - known_tiles) == expected_delta);
            CHECK(reduction.nb_alternatives == 3873724);
            CHECK(reduction.is_fully_reduced);
        }
    }
}

TEST_CASE("Bench linear vs full reduction", "[line_alternatives]")
{
    const LineConstraint constraint(Line::ROW, { 3, 3, 3 });
    const auto min_line_length = constraint.min_line_size();
    CHECK(min_line_length == 11);
    constexpr unsigned int max_extra_zeros = 16;
    FullReductionBuffers buffers(static_cast<unsigned int>(constraint.nb_segments()), min_line_length + max_extra_zeros);
    for (unsigned int extra_zeros = 2; extra_zeros <= max_extra_zeros; extra_zeros += 2)
    {
        const unsigned int nb_alt = ((extra_zeros + 1) * (extra_zeros + 2) * (extra_zeros + 3)) / 6;
        const auto known_tiles    = build_line_from('?', min_line_length + extra_zeros, Line::ROW, 0);
        {
            LineAlternatives::Reduction reduction;
            bool bench_run = false;
            std::string bench_name = "Threes " + std::to_string(nb_alt) + " linear";
            BENCHMARK(std::move(bench_name)) {
                reduction = linear_reduction(constraint, known_tiles);
                bench_run = true;
                return reduction;
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
                bench_run = true;
                return reduction;
            };
            if (bench_run)
            {
                CHECK(reduction.nb_alternatives == nb_alt);
                CHECK(reduction.is_fully_reduced);
            }
        }
        {
            LineAlternatives::Reduction reduction;
            bool bench_run = false;
            std::string bench_name = "Threes " + std::to_string(nb_alt) + " full buf";
            BENCHMARK(std::move(bench_name)) {
                reduction = full_reduction(constraint, known_tiles, &buffers);
                bench_run = true;
                return reduction;
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
