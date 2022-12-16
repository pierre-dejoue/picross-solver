#include <catch2/catch_test_macros.hpp>
#include <picross/picross.h>
#include <utils/test_helpers.h>

#include "line_alternatives.h"
#include "line_constraint.h"

#include <utility>


namespace picross
{
namespace
{
    inline constexpr unsigned int LINE_INDEX = 0;
    inline constexpr bool REVERSED = true;

    LineAlternatives::Reduction full_reduction(const LineConstraint& constraint, const Line& known_tiles, bool reversed = false)
    {
        return LineAlternatives(constraint, known_tiles, reversed).full_reduction();
    }
}

TEST_CASE("full_reduction_of_a_simple_use_case", "[line_alternatives]")
{
    LineConstraint constraint(Line::ROW, { 1, 1 });
    {
        const auto known_tiles = build_line_from("???", Line::ROW, LINE_INDEX);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line == build_line_from("#.#", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 1);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("????", Line::ROW, LINE_INDEX);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line == build_line_from("????", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 3);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("#???", Line::ROW, LINE_INDEX);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line == build_line_from("#.??", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 2);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("#..?", Line::ROW, LINE_INDEX);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line == build_line_from("#..#", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 1);
    }
    {
        const auto known_tiles = build_line_from("??#?", Line::ROW, LINE_INDEX);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line == build_line_from("#.#.", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 1);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("??#?", Line::ROW, LINE_INDEX);
        const auto reduction = full_reduction(constraint, known_tiles, REVERSED);
        CHECK(reduction.reduced_line == build_line_from("#.#.", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 1);
        CHECK(reduction.is_fully_reduced);
    }
}

TEST_CASE("full_reduction_with_a_long_segment_of_ones", "[line_alternatives]")
{
    LineConstraint constraint(Line::ROW, { 6, 1 });
    {
        const auto known_tiles = build_line_from("????????", Line::ROW, LINE_INDEX);
        CHECK(known_tiles.size() == 8);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line == build_line_from("######.#", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 1);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("??????#??", Line::ROW, LINE_INDEX);
        CHECK(known_tiles.size() == 9);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line == build_line_from(".######.#", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 1);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("??????????", Line::ROW, LINE_INDEX);
        CHECK(known_tiles.size() == 10);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line == build_line_from("??####????", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 6);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("??????????", Line::ROW, LINE_INDEX);
        CHECK(known_tiles.size() == 10);
        const auto reduction = full_reduction(constraint, known_tiles, REVERSED);
        CHECK(reduction.reduced_line == build_line_from("??####????", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 6);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("??????#???", Line::ROW, LINE_INDEX);
        CHECK(known_tiles.size() == 10);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line == build_line_from(".?#####???", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 3);
        CHECK(reduction.is_fully_reduced);
    }
}

TEST_CASE("full_reduction_contradictory_use_case", "[line_alternatives]")
{
    LineConstraint constraint(Line::ROW, { 1, 1 });
    {
        const auto known_tiles = build_line_from("?#?", Line::ROW, LINE_INDEX);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line == known_tiles);
        CHECK(reduction.nb_alternatives == 0);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("##??", Line::ROW, LINE_INDEX);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line == known_tiles);
        CHECK(reduction.nb_alternatives == 0);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("##??", Line::ROW, LINE_INDEX);
        const auto reduction = full_reduction(constraint, known_tiles, REVERSED);
        CHECK(reduction.reduced_line == known_tiles);
        CHECK(reduction.nb_alternatives == 0);
        CHECK(reduction.is_fully_reduced);
    }
}

} // namespace picross
