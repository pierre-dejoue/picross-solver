#include <catch2/catch_test_macros.hpp>
#include <picross/picross.h>
#include <utils/test_helpers.h>


namespace picross
{

TEST_CASE("Constraints can use zero to declare an empty row or column", "[solver]")
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

    CHECK(result_1.status == Solver::Status::OK);
    CHECK(result_2.status == Solver::Status::OK);
    REQUIRE(result_1.solutions.size() == 1);
    REQUIRE(result_2.solutions.size() == 1);
    CHECK(result_1.solutions.front().grid == result_2.solutions.front().grid);

    OutputGrid expected = build_output_grid_from(3, 3, R"(
        #.#
        ...
        #.#
    )");

    CHECK(result_1.solutions.front().grid == expected);
}

TEST_CASE("Puzzle: Smile", "[solver]")
{
    // The "Smile" puzzle is a simple pattern that is non line-solvable
    const InputGrid::Constraints rows {
        { 1, 1 },
        { 2 }
    };
    const InputGrid::Constraints cols {
        { 1 },
        { 1 },
        { 1 },
        { 1 }
    };

    InputGrid puzzle(rows, cols, "Smile");

    const auto solver = picross::get_ref_solver();
    const auto result = solver->solve(puzzle);

    CHECK(result.status == Solver::Status::OK);
    REQUIRE(result.solutions.size() == 1);
    const auto& solution = result.solutions.front();

    OutputGrid expected = build_output_grid_from(4, 2, R"(
        #..#
        .##.
    )");

    CHECK(solution.branching_depth == 1);
    CHECK(solution.grid == expected);
}

TEST_CASE("Puzzle: Note", "[solver]")
{
    const InputGrid::Constraints rows {
        { 3 },
        { 1, 1 },
        { 1, 1 },
        { 3 },
        { 3 },
        { }
    };
    const InputGrid::Constraints cols {
        { },
        { 2 },
        { 2 },
        { 5 },
        { 1 },
        { 3 }
    };

    InputGrid puzzle(rows, cols, "Note");

    const auto solver = picross::get_ref_solver();
    const auto result = solver->solve(puzzle);

    CHECK(result.status == Solver::Status::OK);
    REQUIRE(result.solutions.size() == 1);
    const auto& solution = result.solutions.front();

    OutputGrid expected = build_output_grid_from(6, 6, R"(
        ...###
        ...#.#
        ...#.#
        .###..
        .###..
        ......
    )");

    CHECK(solution.branching_depth == 0);
    CHECK(solution.grid == expected);
}

TEST_CASE("Puzzle: Notes", "[solver]")
{
    // A made-up puzzle with two solutions
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

    CHECK(result.status == Solver::Status::OK);

    OutputGridSet expected_solutions {
        build_output_grid_from(10, 10, R"(
            ..###.....
            ..#.#.....
            ..#.#.....
            ###.......
            ###.......
            .......###
            .......#.#
            .......#.#
            .....###..
            .....###..
        )"),
        build_output_grid_from(10, 10, R"(
            .......###
            .......#.#
            .......#.#
            .....###..
            .....###..
            ..###.....
            ..#.#.....
            ..#.#.....
            ###.......
            ###.......
        )")
    };

    REQUIRE(result.solutions.size() == 2);
    OutputGridSet solution_grids { result.solutions[0].grid, result.solutions[1].grid };
    CHECK(solution_grids == expected_solutions);
}

TEST_CASE("Puzzle: Cameraman", "[solver]")
{
    // PicrossDS/Free/07-O-Cameraman.txt
    OutputGrid expected = build_output_grid_from(20, 20, R"(
        ...........#####....
        ..........#######...
        .......###########..
        ..###.###.########..
        .#.#####.#####.###..
        #.#####.####....##..
        ####....#...###.##..
        ##.#.##.##.##....#..
        ...#####..##....#...
        ..#.########...##...
        .#.###..##.###.#....
        .######..###...####.
        .##..##..########..#
        .#.##.#...######...#
        .#.##.##...##...#..#
        .##..####...##..#.##
        ..####..##...#..####
        ...#...####..#.#####
        ...#..#############.
        ....###.###########.
    )", "Cameraman");

    InputGrid puzzle = build_input_grid_from(expected);

    const auto solver = picross::get_ref_solver();
    const auto result = solver->solve(puzzle);

    CHECK(result.status == Solver::Status::OK);
    REQUIRE(result.solutions.size() == 1);
    const auto& solution = result.solutions.front();
    CHECK(solution.branching_depth == 0);
    CHECK(solution.grid == expected);
}

TEST_CASE("Puzzle: 4-DOM", "[solver]")
{
    // Domino pattern
    OutputGrid expected = build_output_grid_from(9, 9, R"(
        ......###
        ........#
        ....###.#
        ......#..
        ..###.#..
        ....#....
        ###.#....
        ..#......
        ..#......
    )", "4-DOM");

    InputGrid puzzle = build_input_grid_from(expected);

    const auto solver = picross::get_ref_solver();
    const auto result = solver->solve(puzzle);

    CHECK(result.status == Solver::Status::OK);
    REQUIRE(result.solutions.size() == 1);
    const auto& solution = result.solutions.front();
    CHECK(solution.branching_depth > 0);
    CHECK(solution.grid == expected);
}

} // namespace picross
