#include <catch2/catch_test_macros.hpp>
#include <picross/picross.h>
#include <utils/input_from_output.h>
#include <utils/test_helpers.h>


// This is the example used in the project README
TEST_CASE("Puzzle: Note", "[solver]")
{
    // Puzzle definition
    const picross::InputGrid::Constraints rows {
        { 3 },
        { 1, 1 },
        { 1, 1 },
        { 3 },
        { 3 },
        { }
    };
    const picross::InputGrid::Constraints cols {
        { },
        { 2 },
        { 2 },
        { 5 },
        { 1 },
        { 3 }
    };
    picross::InputGrid puzzle(rows, cols, "Note");

    // [Optional] Check the puzzle validity
    const auto [check_is_ok, check_msg] = picross::check_input_grid(puzzle);
    REQUIRE(check_is_ok);

    // Solve it
    const auto solver = picross::get_ref_solver();
    const auto result = solver->solve(puzzle);

    CHECK(result.status == picross::Solver::Status::OK);
    REQUIRE(result.solutions.size() == 1);
    const auto& solution = result.solutions.front();

    picross::OutputGrid expected = picross::build_output_grid_from(6, 6, R"(
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

    const auto solver = get_ref_solver();
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

    const auto solver = get_ref_solver();
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

    const auto solver = get_ref_solver();
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

TEST_CASE("Puzzle: Flip", "[solver]")
{
    // Example in the paper by Ueda & Nagao, "NP-completeness Results for NONOGRAM via Parsimonious Reductions", 1996
    const InputGrid::Constraints rows {
        { 2, 3 },
        { 3 },
        { 1, 1, 1 },
        { 1, 1 }
    };
    const InputGrid::Constraints cols {
        { 1, 1 },
        { 2, 1 },
        { 2 },
        { 2 },
        { 1, 1 },
        { 1, 1 }
    };

    InputGrid puzzle(rows, cols, "Flip");

    //
    // 1. Full resolution (with branching)
    //
    {
        const auto solver = get_ref_solver();
        const auto result = solver->solve(puzzle);

        CHECK(result.status == Solver::Status::OK);

        OutputGridSet expected_solutions {
            build_output_grid_from(6, 4, R"(
                ##.###
                .###..
                #.#..#
                .#..#.
            )"),
            build_output_grid_from(6, 4, R"(
                ##.###
                .###..
                #.#.#.
                .#...#
            )")
        };

        REQUIRE(result.solutions.size() == 2);
        OutputGridSet solution_grids { result.solutions[0].grid, result.solutions[1].grid };
        CHECK(solution_grids == expected_solutions);
        CHECK(result.solutions[0].branching_depth == 1);
        CHECK(result.solutions[0].partial == false);
        CHECK(result.solutions[1].branching_depth == 1);
        CHECK(result.solutions[1].partial == false);
        CHECK(is_solution(puzzle, result.solutions[0].grid));
        CHECK(is_solution(puzzle, result.solutions[1].grid));
        CHECK(list_incompatible_lines(puzzle, result.solutions[0].grid).empty());
        CHECK(list_incompatible_lines(puzzle, result.solutions[1].grid).empty());
    }

    //
    // 2. Line solve (no branching allowed)
    //
    {
        const auto solver = get_line_solver();
        const auto result = solver->solve(puzzle);

        CHECK(result.status == Solver::Status::NOT_LINE_SOLVABLE);

        const OutputGrid expected_partial_solution = build_output_grid_from(6, 4, R"(
                ##.###
                .###..
                #.#.??
                .#..??
            )");

        REQUIRE(result.solutions.size() == 1);
        CHECK(result.solutions[0].grid == expected_partial_solution);
        CHECK(result.solutions[0].branching_depth == 0);
        CHECK(result.solutions[0].partial == true);
        CHECK(!is_solution(puzzle, result.solutions[0].grid));
    }
}

TEST_CASE("Puzzle: A piece of Centerpiece", "[solver]")
{
    // A subpart of the puzzle "Centerpiece" webpbn-10810. It has three solutions.
    const InputGrid::Constraints rows {
        { 1, 1 },
        { 2 },
        { 2 },
        { 1, 1 }
    };
    const InputGrid::Constraints cols {
        { 1, 1 },
        { 2 },
        { 2 },
        { 1, 1 }
    };

    InputGrid puzzle(rows, cols, "Piece");

    const auto solver = get_ref_solver();
    const auto result = solver->solve(puzzle);

    CHECK(result.status == Solver::Status::OK);

    OutputGridSet expected_solutions {
        build_output_grid_from(4, 4, R"(
            #..#
            .##.
            .##.
            #..#
        )"),
        build_output_grid_from(4, 4, R"(
            .#.#
            ##..
            ..##
            #.#.
        )"),
        build_output_grid_from(4, 4, R"(
            #.#.
            ..##
            ##..
            .#.#
        )")
    };

    REQUIRE(result.solutions.size() == 3);
    OutputGridSet solution_grids { result.solutions[0].grid, result.solutions[1].grid, result.solutions[2].grid};
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

    const auto solver = get_ref_solver();
    const auto result = solver->solve(puzzle);

    CHECK(result.status == Solver::Status::OK);
    REQUIRE(result.solutions.size() == 1);
    const auto& solution = result.solutions.front();
    CHECK(solution.branching_depth == 0);
    CHECK(solution.grid == expected);
}

TEST_CASE("Puzzle: 3-DOM", "[solver]")
{
    // Domino pattern (not line-solvable)
    OutputGrid expected = build_output_grid_from(7, 7, R"(
        ....###
        ......#
        ..###.#
        ....#..
        ###.#..
        ..#....
        ..#....
    )", "3-DOM");

    InputGrid puzzle = build_input_grid_from(expected);

    const auto solver = get_ref_solver();
    const auto result = solver->solve(puzzle);

    CHECK(result.status == Solver::Status::OK);
    REQUIRE(result.solutions.size() == 1);
    const auto& solution = result.solutions.front();
    CHECK(solution.branching_depth > 0);
    CHECK(solution.grid == expected);
}

} // namespace picross
