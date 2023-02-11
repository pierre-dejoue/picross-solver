#include <picross/picross.h>

#include <cassert>
#include <cstdlib>
#include <iostream>

int main()
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
    if (!check_is_ok)
        return EXIT_FAILURE;

    // Solve the puzzle
    const auto solver = picross::get_ref_solver();
    assert(solver);
    constexpr unsigned int max_nb_solutions = 2;
    const auto result = solver->solve(puzzle, max_nb_solutions);
    if (result.status != picross::Solver::Status::OK)
        return EXIT_FAILURE;

    // Print out the solutions
    assert(!result.solutions.empty());
    for (const auto& solution : result.solutions)
        std::cout << solution.grid << std::endl;

    return EXIT_SUCCESS;
}
