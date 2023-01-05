#include <catch2/catch_test_macros.hpp>
#include <picross/picross.h>
#include <utils/test_helpers.h>

#include "binomial.h"
#include "line.h"
#include "line_alternatives.h"
#include "line_constraint.h"
#include "line_span.h"

#include <utility>


namespace picross
{
namespace
{
    inline constexpr unsigned int LINE_INDEX = 0;

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
}

TEST_CASE("full_reduction_of_a_simple_use_case", "[line_alternatives]")
{
    const LineConstraint constraint(Line::ROW, { 1, 1 });
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
}

TEST_CASE("full_reduction_with_a_long_segment_of_ones", "[line_alternatives]")
{
    const LineConstraint constraint(Line::ROW, { 6, 1 });
    {
        const auto known_tiles = build_line_from('?', 8, Line::ROW, LINE_INDEX);
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
        const auto known_tiles = build_line_from('?', 10, Line::ROW, LINE_INDEX);
        CHECK(known_tiles.size() == 10);
        const auto reduction = full_reduction(constraint, known_tiles);
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

TEST_CASE("full_reduction_contradictory_use_case_a", "[line_alternatives]")
{
    const LineConstraint constraint(Line::ROW, { 1 });
    {
        const auto known_tiles = build_line_from("..", Line::ROW, LINE_INDEX);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line.type() == known_tiles.type());
        CHECK(reduction.reduced_line.index() == known_tiles.index());
        CHECK(reduction.reduced_line.size() == known_tiles.size());
        CHECK(reduction.nb_alternatives == 0);
        CHECK(reduction.is_fully_reduced);
    }
}

TEST_CASE("full_reduction_contradictory_use_case_b", "[line_alternatives]")
{
    const LineConstraint constraint(Line::ROW, { 1, 1 });
    {
        const auto known_tiles = build_line_from("?#?", Line::ROW, LINE_INDEX);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line.type() == known_tiles.type());
        CHECK(reduction.reduced_line.index() == known_tiles.index());
        CHECK(reduction.reduced_line.size() == known_tiles.size());
        CHECK(reduction.nb_alternatives == 0);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("##??", Line::ROW, LINE_INDEX);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line.type() == known_tiles.type());
        CHECK(reduction.reduced_line.index() == known_tiles.index());
        CHECK(reduction.reduced_line.size() == known_tiles.size());
        CHECK(reduction.nb_alternatives == 0);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("#...", Line::ROW, LINE_INDEX);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line.type() == known_tiles.type());
        CHECK(reduction.reduced_line.index() == known_tiles.index());
        CHECK(reduction.reduced_line.size() == known_tiles.size());
        CHECK(reduction.nb_alternatives == 0);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from(".#.", Line::ROW, LINE_INDEX);
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line.type() == known_tiles.type());
        CHECK(reduction.reduced_line.index() == known_tiles.index());
        CHECK(reduction.reduced_line.size() == known_tiles.size());
        CHECK(reduction.nb_alternatives == 0);
        CHECK(reduction.is_fully_reduced);
    }
}

TEST_CASE("partial_reduction_symetrical_use_case", "[line_alternatives]")
{
    const LineConstraint constraint(Line::ROW, { 3, 3, 3 });
    const auto known_tiles_11 = build_line_from('?', 11, Line::ROW, LINE_INDEX);
    const auto known_tiles_12 = build_line_from('?', 12, Line::ROW, LINE_INDEX);
    {
        const auto reduction = partial_reduction(constraint, known_tiles_11, 1);
        CHECK(reduction.reduced_line == build_line_from("###.???.###", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 1);
        CHECK_FALSE(reduction.is_fully_reduced);
    }
    {
        const auto reduction = partial_reduction(constraint, known_tiles_11, 2);
        CHECK(reduction.reduced_line == build_line_from("###.###.###", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 1);
        CHECK(reduction.is_fully_reduced);
    }
    {

        const auto reduction = partial_reduction(constraint, known_tiles_12, 1);
        CHECK(reduction.reduced_line == build_line_from("?##??????##?", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 4);
        CHECK_FALSE(reduction.is_fully_reduced);
    }
    {
        const auto reduction = partial_reduction(constraint, known_tiles_12, 2);
        CHECK(reduction.reduced_line == build_line_from("?##??##??##?", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 4);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("???#????????", Line::ROW, LINE_INDEX);
        CHECK(known_tiles.size() == 12);
        const auto reduction = partial_reduction(constraint, known_tiles, 1);
        CHECK(reduction.reduced_line == build_line_from(".###.????##?", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 1);
        CHECK_FALSE(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("???#?????????", Line::ROW, LINE_INDEX);
        CHECK(known_tiles.size() == 13);
        const auto reduction = partial_reduction(constraint, known_tiles, 1);
        CHECK(reduction.reduced_line == build_line_from(".?##??????#??", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 4);
        CHECK_FALSE(reduction.is_fully_reduced);
    }
}

TEST_CASE("partial_reduction_with_a_long_segment", "[line_alternatives]")
{
    const LineConstraint constraint(Line::ROW, { 6, 1, 1 });
    {
        const auto known_tiles = build_line_from('?', 10, Line::ROW, LINE_INDEX);
        const auto reduction = partial_reduction(constraint, known_tiles, 1);
        CHECK(reduction.reduced_line == build_line_from("######.?.#", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 1);
        CHECK_FALSE(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("??????#????", Line::ROW, LINE_INDEX);
        CHECK(known_tiles.size() == 11);
        const auto reduction = partial_reduction(constraint, known_tiles, 1);
        CHECK(reduction.reduced_line == build_line_from(".######.???", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 1);
        CHECK_FALSE(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from('?', 12, Line::ROW, LINE_INDEX);
        const auto reduction = partial_reduction(constraint, known_tiles, 1);
        CHECK(reduction.reduced_line == build_line_from("??####??????", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 10);
        CHECK_FALSE(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("??????#?????", Line::ROW, LINE_INDEX);
        CHECK(known_tiles.size() == 12);
        const auto reduction = partial_reduction(constraint, known_tiles, 1);
        CHECK(reduction.reduced_line == build_line_from(".?#####?????", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 4);
        CHECK_FALSE(reduction.is_fully_reduced);
    }
    {
        const auto known_tiles = build_line_from("??????#?????", Line::ROW, LINE_INDEX);
        CHECK(known_tiles.size() == 12);
        const auto reduction = partial_reduction(constraint, known_tiles, 2);
        CHECK(reduction.reduced_line == build_line_from(".?#####?????", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 4);
        CHECK(reduction.is_fully_reduced);
    }
}

TEST_CASE("reduction_of_a_completed_line", "[line_alternatives]")
{
    const LineConstraint constraint(Line::ROW, { 4, 1, 3, 1 });
    const auto known_tiles = build_line_from("..####....#...###.#...", Line::ROW, LINE_INDEX);
    {
        const auto reduction = full_reduction(constraint, known_tiles);
        CHECK(reduction.reduced_line == known_tiles);
        CHECK(reduction.nb_alternatives == 1);
        CHECK(reduction.is_fully_reduced);
    }
    {
        const auto reduction = partial_reduction(constraint, known_tiles, 1);
        CHECK(reduction.reduced_line == known_tiles);
        CHECK(reduction.nb_alternatives == 1);
        CHECK(reduction.is_fully_reduced);
    }
}

TEST_CASE("partial_reductions_of_a_trivially_solvable_line", "[line_alternatives]")
{
    const LineConstraint constraint(Line::ROW, { 4, 1, 2, 1, 3, 1 });
    const auto min_line_size = constraint.min_line_size();
    Line known_tiles = build_line_from('?', min_line_size, Line::ROW, LINE_INDEX);
    LineAlternatives line_alternatives(constraint, known_tiles, get_binomial());
    {
        const auto reduction = line_alternatives.partial_reduction(1);
        CHECK(reduction.reduced_line == build_line_from("####.??????????.#", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 1);
        CHECK_FALSE(reduction.is_fully_reduced);
        known_tiles = reduction.reduced_line;
    }
        {
        const auto reduction = line_alternatives.partial_reduction(1);
        CHECK(reduction.reduced_line == build_line_from("####.#.????.###.#", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 1);
        CHECK_FALSE(reduction.is_fully_reduced);
        known_tiles = reduction.reduced_line;
    }
    {
        const auto reduction = line_alternatives.partial_reduction(1);
        CHECK(reduction.reduced_line == build_line_from("####.#.##.#.###.#", Line::ROW, LINE_INDEX));
        CHECK(reduction.nb_alternatives == 1);
        CHECK(reduction.is_fully_reduced);
    }
}

// Test based on the puzzle webpbn-10810.non "Centerpiece"
TEST_CASE("centerpiece-webpbn-10810-row-8", "[line_alternatives]")
{
    constexpr auto MAX = BinomialCoefficients::overflowValue();

    const LineConstraint constraint(Line::ROW, { 2,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,2 });
    const auto min_line_size = constraint.min_line_size();
    CHECK(min_line_size == 51);

    Line known_tiles = build_line_from('?', 60, Line::ROW, 8);
    LineAlternatives line_alternatives(constraint, known_tiles, get_binomial());
    {
        const auto partial_reduction = line_alternatives.partial_reduction(1);
        CHECK(partial_reduction.reduced_line == known_tiles);
        CHECK(partial_reduction.nb_alternatives == 20160075);
        CHECK_FALSE(partial_reduction.is_fully_reduced);
    }
    copy_line_from_line_span(known_tiles, build_line_from("#?.??????????????????????????????????????????????????????.?#", Line::ROW, 8));
    {
        const Line expected = build_line_from("##.??????????????????????????????????????????????????????.##", Line::ROW, 8);
        const auto partial_reduction = line_alternatives.partial_reduction(1);
        CHECK(partial_reduction.reduced_line == expected);
        CHECK(partial_reduction.nb_alternatives == 14307150);
        CHECK_FALSE(partial_reduction.is_fully_reduced);
    }
    copy_line_from_line_span(known_tiles, build_line_from("##.???#.????????????????????????????????????????????.#???.##", Line::ROW, 8));
    {
        const Line expected = build_line_from("##.####.????????????????????????????????????????????.####.##", Line::ROW, 8);
        const auto partial_reduction = line_alternatives.partial_reduction(1);
        CHECK(partial_reduction.reduced_line == expected);
        CHECK(partial_reduction.nb_alternatives == 6906900);
        CHECK_FALSE(partial_reduction.is_fully_reduced);
    }
    copy_line_from_line_span(known_tiles, build_line_from("##.####.????.????.????.????.????.????.????.????.????.####.##", Line::ROW, 8));
    {
        const Line partial_expected = build_line_from("##.####.????????????????????????????????????????????.####.##", Line::ROW, 8);
        const auto partial_reduction = line_alternatives.partial_reduction(1);
        CHECK(partial_reduction.reduced_line == partial_expected);
        CHECK(partial_reduction.nb_alternatives == 4660490);
        CHECK_FALSE(partial_reduction.is_fully_reduced);
        const auto full_reduction = line_alternatives.full_reduction();
        CHECK(full_reduction.reduced_line == known_tiles);
        CHECK(full_reduction.nb_alternatives == 19683);
        CHECK(full_reduction.is_fully_reduced);
    }
}

// Test based on the puzzle webpbn-10810.non "Centerpiece"
TEST_CASE("centerpiece-webpbn-10810-row-9", "[line_alternatives]")
{
    constexpr auto MAX = BinomialCoefficients::overflowValue();

    const LineConstraint constraint(Line::ROW, { 1,1,1,2,2,2,2,2,2,2,2,2,1,1,1 });
    const auto min_line_size = constraint.min_line_size();
    CHECK(min_line_size == 38);

    Line known_tiles = build_line_from('?', 60, Line::ROW, 9);
    LineAlternatives line_alternatives(constraint, known_tiles, get_binomial());
    {
        const auto partial_reduction = line_alternatives.partial_reduction(1);
        CHECK(partial_reduction.reduced_line == known_tiles);
        CHECK(partial_reduction.nb_alternatives == MAX);
        CHECK_FALSE(partial_reduction.is_fully_reduced);
    }
    copy_line_from_line_span(known_tiles, build_line_from("?#?#??#??????????????????????????????????????????????#??#?#?", Line::ROW, 9));
    {
        const Line expected = build_line_from(".#.??????????????????????????????????????????????????????.#.", Line::ROW, 9);
        const auto partial_reduction = line_alternatives.partial_reduction(1);
        CHECK(partial_reduction.reduced_line == expected);
        CHECK(partial_reduction.nb_alternatives == 2319959400); // 0x8a47c568
        CHECK_FALSE(partial_reduction.is_fully_reduced);
    }
    copy_line_from_line_span(known_tiles, build_line_from(".#.#??#??????????????????????????????????????????????#??#.#.", Line::ROW, 9));
    {
        const Line expected = build_line_from(".#.#.??????????????????????????????????????????????????.#.#.", Line::ROW, 9);
        const auto partial_reduction = line_alternatives.partial_reduction(1);
        CHECK(partial_reduction.reduced_line == expected);
        CHECK(partial_reduction.nb_alternatives == 225792840);  // 0x0d755348
        CHECK_FALSE(partial_reduction.is_fully_reduced);
    }
    copy_line_from_line_span(known_tiles, build_line_from(".#.#.?#??????????????????????????????????????????????#?.#.#.", Line::ROW, 9));

    {
        const Line expected = build_line_from(".#.#..#.????????????????????????????????????????????.#..#.#.", Line::ROW, 9);
        const auto partial_reduction = line_alternatives.partial_reduction(1);
        CHECK(partial_reduction.reduced_line == expected);
        CHECK(partial_reduction.nb_alternatives == 20030010);   // 0x0131a23a
        CHECK_FALSE(partial_reduction.is_fully_reduced);
    }
    copy_line_from_line_span(known_tiles, build_line_from(".#.#..#.?????????.????.????.????.????.????.?????????.#..#.#.", Line::ROW, 9));
    {
        const Line partial_expected = build_line_from(".#.#..#.????????????????????????????????????????????.#..#.#.", Line::ROW, 9);
        const auto partial_reduction = line_alternatives.partial_reduction(1);
        CHECK(partial_reduction.reduced_line == partial_expected);
        CHECK(partial_reduction.nb_alternatives == 4616974);    // 0x0046730e
        CHECK_FALSE(partial_reduction.is_fully_reduced);
        const auto full_reduction = line_alternatives.full_reduction();
        CHECK(full_reduction.reduced_line == known_tiles);
        CHECK(full_reduction.nb_alternatives == 123147);
        CHECK(full_reduction.is_fully_reduced);
    }
}

} // namespace picross
