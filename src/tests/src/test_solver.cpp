#include <catch2/catch_test_macros.hpp>
#include <picross/picross.h>

namespace picross
{

TEST_CASE("Puzzle: Note", "[solver]")
{
    const InputGrid::Constraints rows {
        { 3 },
        { 1, 1 },
        { 1, 1 },
        { 3 },
        { 3 }
    };
    const InputGrid::Constraints cols {
        { 2 },
        { 2 },
        { 5 },
        { 1 },
        { 3 }
    };

    InputGrid puzzle(rows, cols, "Note");

    const auto solver = picross::get_ref_solver();
    const auto result = solver->solve(puzzle);

    REQUIRE(result.status == Solver::Status::OK);
    REQUIRE(result.solutions.size() == 1);
}

TEST_CASE("Puzzle: Notes", "[solver]")
{
    const InputGrid::Constraints rows {
        { 3 },
        { 1, 1 },
        { 1, 1 },
        { 3 },
        { 3 },
        { 3 },
        { 1, 1 },
        { 1, 1 },
        { 3 },
        { 3 }
    };
    const InputGrid::Constraints cols {
        { 2 },
        { 2 },
        { 5 },
        { 1 },
        { 3 },
        { 2 },
        { 2 },
        { 5 },
        { 1 },
        { 3 }
    };

    InputGrid puzzle(rows, cols, "Notes");

    const auto solver = picross::get_ref_solver();
    const auto result = solver->solve(puzzle);

    REQUIRE(result.status == Solver::Status::OK);
    REQUIRE(result.solutions.size() == 2);
}

TEST_CASE("Constraints: Use zero to declare an empty line", "[solver]")
{
    // In the definition of a Picross puzzle, an empty line can be declared by either an empty vector, or a vector with a single zero element
    const InputGrid::Constraints rows_wo_zero {
        { 1, 1 },
        { },
        { 1, 1 }
    };
    const InputGrid::Constraints cols_wo_zero = rows_wo_zero;

    const InputGrid::Constraints rows_w_zero {
        { 1, 1 },
        { 0 },
        { 1, 1 }
    };
    const InputGrid::Constraints cols_w_zero = rows_w_zero;

    InputGrid puzzle_1(rows_wo_zero, cols_wo_zero, "");
    InputGrid puzzle_2(rows_w_zero, cols_w_zero, "");

    const auto solver = picross::get_ref_solver();
    const auto result_1 = solver->solve(puzzle_1);
    const auto result_2 = solver->solve(puzzle_2);

    REQUIRE(result_1.status == Solver::Status::OK);
    REQUIRE(result_1.solutions.size() == 1);
    REQUIRE(result_2.status == Solver::Status::OK);
    REQUIRE(result_2.solutions.size() == 1);
    REQUIRE(result_1.solutions.front().grid == result_2.solutions.front().grid);
}

} // namespace picross
