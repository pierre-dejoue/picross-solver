#include <catch2/benchmark/catch_benchmark_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <picross/picross.h>
#include <utils/input_from_output.h>
#include <utils/test_helpers.h>


namespace picross
{

TEST_CASE("Benchmark", "[solver]")
{
    {
        // Edge pattern. Source: webpbn-00023
        OutputGrid expected = build_output_grid_from(10, 11, R"(
            ...#......
            .###......
            .#........
            ##........
            .....#....
            ...###....
            ...###....
            .....#....
            .......##.
            .......##.
            ......####
        )", "Edge");

        InputGrid puzzle = build_input_grid_from(expected);

        const auto solver = picross::get_ref_solver();
        const auto result = solver->solve(puzzle);

        CHECK(result.status == Solver::Status::OK);
        REQUIRE(result.solutions.size() == 1);
        const auto& solution = result.solutions.front();
        CHECK(solution.branching_depth > 0);
        CHECK(solution.grid == expected);

        BENCHMARK("Edge pattern"){ return solver->solve(puzzle); };
    }

    {
        // Domino pattern
        OutputGrid expected = build_output_grid_from(11, 11, R"(
            ........###
            ..........#
            ......###.#
            ........#..
            ....###.#..
            ......#....
            ..###.#....
            ....#......
            ###.#......
            ..#........
            ..#........
        )", "5-DOM");

        InputGrid puzzle = build_input_grid_from(expected);

        const auto solver = picross::get_ref_solver();
        const auto result = solver->solve(puzzle);

        CHECK(result.status == Solver::Status::OK);
        REQUIRE(result.solutions.size() == 1);
        const auto& solution = result.solutions.front();
        CHECK(solution.branching_depth > 0);
        CHECK(solution.grid == expected);

        BENCHMARK("5-DOM"){ return solver->solve(puzzle); };
    }
    {
        // Source: webpbn-02413
        OutputGrid expected = build_output_grid_from(20, 20, R"(
            .#.###..##.........#
            #.##...##...........
            ###...####..........
            .##.###..##.........
            ##..#....######.....
            ##..#############..#
            .#.....#....########
            .##....#.#...#######
            ..#...##.##..##..###
            ###..#.......#.#.###
            .#..##.......#.#.###
            .##.#...#........###
            .#..#####......#####
            ..#....#........###.
            .....####.......##..
            .##.##..#......##..#
            .##.#..##....###..##
            .####..#..######..#.
            .###...####.###..##.
            ...........####.##..
        )", "Smoke");

        InputGrid puzzle = build_input_grid_from(expected);

        const auto solver = picross::get_ref_solver();
        const auto result = solver->solve(puzzle);

        CHECK(result.status == Solver::Status::OK);
        REQUIRE(result.solutions.size() == 1);
        const auto& solution = result.solutions.front();
        CHECK(solution.branching_depth > 0);
        CHECK(solution.grid == expected);

        BENCHMARK("Smoke"){ return solver->solve(puzzle); };
    }
    {
        // Source: webpbn-00529
        OutputGrid expected = build_output_grid_from(45, 45, R"(
            #######...#......#......#.......#.......#....
            ###.#.###.#.####.#.####.#.#####.#.#####.#.##.
            #.#.#.###.#.####.#.####.#.#####.#.#####.#.##.
            ##.#..##..#......#......#.......#..######.##.
            ###...##############################.#.#####.
            #.....#####.....########.#.#.#######.#.#.###.
            ###....####.....########.....#.#####..#.##...
            .###...####################...######...######
            .###...###.......#######...##..#####.....#...
            .###....###.#.#.#########.#.#.#####...######.
            ...##...###.....########....#..###...####.##.
            #####...###.#...##########...####...#####.##.
            ....#....##.###.########....####...######....
            .##.##...###...###########.........##########
            .##.##....###..##########.........#######....
            .##.###......#...#######.....############.##.
            .##.###......#.....####.......###########.##.
            ....####.....#......##...#....###########.##.
            #########....#..........##......#########....
            ....######..##..#......####.......###########
            .##.#####...#...##....######........######...
            .######....##..####..########...........####.
            .####.....##..################.............#.
            .##......##...###############.............##.
            .###....##...###############............####.
            ..###..###...#############............####...
            ####........############............#########
            ..#..........#########.........##########....
            .##.....#.....#################...#######.##.
            .##.....##.....########....###...########.##.
            .##.....###.....######....###....########.##.
            .##.....####.....#####...####.....#######.##.
            ..##....#####....#####....####.....######....
            ####.....####.....#####....####.....#########
            ...#.....####.....######....####.....####....
            .####....###......######.....####.....###.##.
            .##.#.....##......#######.....####...####.##.
            .##.##....##.....#########...#####..#####.##.
            .##.##.....##...##########..######.######....
            ....###.....##.#.#########.##################
            ########...####...#######################....
            ....#..##.#...##..##......#.......#.....#.##.
            .##.#.####.....##..#.####.#.#####.#.###.#.##.
            .##.#.#####.....####.####.#.#####.#.###.#.##.
            ....#.....##########......#.......#.....#....
        )", "Swing");

        InputGrid puzzle = build_input_grid_from(expected);

        const auto solver = picross::get_ref_solver();
        const auto result = solver->solve(puzzle);

        CHECK(result.status == Solver::Status::OK);
        REQUIRE(result.solutions.size() == 1);
        const auto& solution = result.solutions.front();
        CHECK(solution.branching_depth == 0);
        CHECK(solution.grid == expected);

        BENCHMARK("Swing"){ return solver->solve(puzzle); };
    }
}

} // namespace picross
