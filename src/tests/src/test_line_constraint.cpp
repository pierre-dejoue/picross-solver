#include <catch2/catch_test_macros.hpp>
#include <picross/picross.h>
#include <utils/test_helpers.h>

#include "line_constraint.h"


namespace picross
{
namespace
{
inline constexpr unsigned int LINE_INDEX = 0;
}

TEST_CASE("compute_min_line_size", "[line_constraint]")
{
    CHECK(compute_min_line_size({ }) == 0);
    CHECK(compute_min_line_size({ 0 }) == 0);
    CHECK(compute_min_line_size({ 1 }) == 1);
    CHECK(compute_min_line_size({ 1, 1 }) == 3);
    CHECK(compute_min_line_size({ 5, 7 }) == 13);
}

TEST_CASE("line_trivial_nb_alternatives", "[line_constraint]")
{
    BinomialCoefficients::Cache binomial;
    LineConstraint constraint(Line::ROW, { 1, 1 });
    CHECK_THROWS(constraint.line_trivial_nb_alternatives(0, binomial));
    CHECK_THROWS(constraint.line_trivial_nb_alternatives(1, binomial));
    CHECK_THROWS(constraint.line_trivial_nb_alternatives(2, binomial));
    CHECK(constraint.line_trivial_nb_alternatives(3, binomial) == 1);
    CHECK(constraint.line_trivial_nb_alternatives(4, binomial) == 3);
    CHECK(constraint.line_trivial_nb_alternatives(5, binomial) == 6);
}

TEST_CASE("line_trivial_reduction", "[line_constraint]")
{
    LineConstraint constraint(Line::ROW, { 1, 1 });
    CHECK_THROWS(constraint.line_trivial_reduction(0, LINE_INDEX));
    CHECK_THROWS(constraint.line_trivial_reduction(1, LINE_INDEX));
    CHECK_THROWS(constraint.line_trivial_reduction(2, LINE_INDEX));
    CHECK(constraint.line_trivial_reduction(3, LINE_INDEX) == build_line_from("#.#", Line::ROW, LINE_INDEX));
    CHECK(constraint.line_trivial_reduction(4, LINE_INDEX) == build_line_from("????", Line::ROW, LINE_INDEX));
    CHECK(constraint.line_trivial_reduction(5, LINE_INDEX) == build_line_from("?????", Line::ROW, LINE_INDEX));
}

TEST_CASE("empty_line", "[line_constraint]")
{
    BinomialCoefficients::Cache binomial;
    for (const auto& constraint: { LineConstraint(Line::ROW, { }), LineConstraint(Line::ROW, { 0 }) })
    {
        CHECK(constraint.line_trivial_nb_alternatives(0, binomial) == 1);
        CHECK(constraint.line_trivial_nb_alternatives(1, binomial) == 1);
        CHECK(constraint.line_trivial_nb_alternatives(2, binomial) == 1);
        CHECK(constraint.line_trivial_nb_alternatives(3, binomial) == 1);
        CHECK(constraint.line_trivial_reduction(0, LINE_INDEX) == build_line_from("", Line::ROW, LINE_INDEX));
        CHECK(constraint.line_trivial_reduction(1, LINE_INDEX) == build_line_from(".", Line::ROW, LINE_INDEX));
        CHECK(constraint.line_trivial_reduction(2, LINE_INDEX) == build_line_from("..", Line::ROW, LINE_INDEX));
        CHECK(constraint.line_trivial_reduction(3, LINE_INDEX) == build_line_from("...", Line::ROW, LINE_INDEX));
    }
}

TEST_CASE("full_line", "[line_constraint]")
{
    BinomialCoefficients::Cache binomial;
    for (unsigned int line_sz = 0; line_sz < 10; line_sz++)
    {
        LineConstraint constraint(Line::ROW, { line_sz });
        CHECK(constraint.line_trivial_nb_alternatives(line_sz, binomial) == 1);
        CHECK(constraint.line_trivial_reduction(line_sz, LINE_INDEX) == build_line_from(std::string(line_sz, '#'), Line::ROW, LINE_INDEX));
    }
}

TEST_CASE("partial_reduction", "[line_constraint]")
{
    BinomialCoefficients::Cache binomial;
    LineConstraint constraint(Line::ROW, { 6, 1 });

    CHECK_THROWS(constraint.line_trivial_nb_alternatives(7, binomial));
    CHECK(constraint.line_trivial_nb_alternatives(8, binomial) == 1);
    CHECK(constraint.line_trivial_nb_alternatives(9, binomial) == 3);
    CHECK(constraint.line_trivial_nb_alternatives(10, binomial) == 6);
    CHECK(constraint.line_trivial_nb_alternatives(11, binomial) == 10);

    CHECK_THROWS(constraint.line_trivial_reduction(7, LINE_INDEX));
    CHECK(constraint.line_trivial_reduction(8, LINE_INDEX)  == build_line_from("######.#", Line::ROW, LINE_INDEX));
    CHECK(constraint.line_trivial_reduction(9, LINE_INDEX)  == build_line_from("?#####???", Line::ROW, LINE_INDEX));
    CHECK(constraint.line_trivial_reduction(10, LINE_INDEX) == build_line_from("??####????", Line::ROW, LINE_INDEX));
    CHECK(constraint.line_trivial_reduction(11, LINE_INDEX) == build_line_from("???###?????", Line::ROW, LINE_INDEX));
    CHECK(constraint.line_trivial_reduction(12, LINE_INDEX) == build_line_from("????##??????", Line::ROW, LINE_INDEX));
    CHECK(constraint.line_trivial_reduction(13, LINE_INDEX) == build_line_from("?????#???????", Line::ROW, LINE_INDEX));
    CHECK(constraint.line_trivial_reduction(14, LINE_INDEX) == build_line_from("??????????????", Line::ROW, LINE_INDEX));
}

} // namespace picross
