/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PUBLIC API of the Picross solver
 *
 *   - This is the MAIN include file of the library
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include "picross_input_grid.h"
#include "picross_io.h"
#include "picross_observer.h"
#include "picross_output_grid.h"
#include "picross_stats.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>


namespace picross
{

/*
 * Get the version of the library
 */
std::string_view get_version_string();


/*
 * Solver interface
 */
class Solver
{
public:
    virtual ~Solver() = default;

    // Return status of the solver
    enum class Status
    {
        OK,                     // Solver process completed and one or more solutions found
        ABORTED,                // Solver process aborted
        CONTRADICTORY_GRID,     // Not solvable
        NOT_LINE_SOLVABLE       // Not line solvable (i.e. not solvable without a branching algorithm)
    };

    struct Solution
    {
        OutputGrid grid;
        unsigned int branching_depth;
        bool partial;
    };
    using Solutions = std::vector<Solution>;

    // Solver result
    struct Result
    {
        Status status;
        Solutions solutions;
    };

    //
    // Solve a grid
    //
    // By default the solver will look for all the solutions of the input grid.
    // The optional argument max_nb_solutions can be used to limit the number of solutions discovered by the algorithm.
    // max_nb_solutions = 0 means no limit
    //
    // If the solver returns with status Status::NOT_LINE_SOLVABLE, the partially solved grid is part of the
    // solutions
    //
    virtual Result solve(const InputGrid& input_grid, unsigned int max_nb_solutions = 0u) const = 0;


    //
    // Solve a grid
    //
    // The callback solution_found gets called each time the solver finds a solution.
    // If the callback returns true, the solver process continue, otherwise it will stop and this
    // method will return with status Status::ABORTED
    //
    // If the solver returns with status Status::NOT_LINE_SOLVABLE, the partially solved grid is passed to the
    // callback function solution_found, with the partial flag set to true
    //
    using SolutionFound = std::function<bool(Solution&&)>;
    virtual Status solve(const InputGrid& input_grid, SolutionFound solution_found) const = 0;


    //
    // Set an optional observer on the solver
    //
    // It will be notified of certain events, allowing the user to follow the solving process step by step.
    //
    virtual void set_observer(Observer observer) = 0;


    //
    // Set optional stats
    //
    // The stat object is reset at the beginning of each call to Solver::solve()
    //
    virtual void set_stats(GridStats& stats) = 0;


    //
    // Set an abort function
    //
    // If set, the solver will regularly call this function and abort its processing in case it returns true.
    // The solver will return the fully completed solutions it has already computed.
    //
    using Abort = std::function<bool()>;
    virtual void set_abort_function(Abort abort) = 0;
};

std::ostream& operator<<(std::ostream& out, Solver::Status status);


/*
 * Factory for the reference grid solver
 */
std::unique_ptr<Solver> get_ref_solver();


/*
 * Factory for a line solver
 */
std::unique_ptr<Solver> get_line_solver();


/*
 * Validation code:
 *
 *  -1  ERR     The input grid is invalid
 *   0  ZERO    No solution found
 *   1  OK      Valid grid with a unique solution
 *   2  MULT    The solution is not unique
 */
using ValidationCode = int;
std::string_view str_validation_code(ValidationCode code);


/*
 * Validation method: check that the input grid is valid and has a unique solution.
 *
 * Return the validation code (ERR, ZERO, OK, MULT), the branching depth required to solve it (which determines
 * if the grid is line solvable or not), and an optional message regarding the grid validation process.
 *
 * NB: The case where the input grid's constraint are not compatible is reported as an ERR (-1): contradictory grid.
 *     An input grid for which the constraints are coherent should have at least one solution. Therefore the validation
 *     code ZERO (0) can only be returned if the solver just gave up finding a solution.
 */
struct ValidationResult
{
    ValidationCode code;
    unsigned int branching_depth;
    std::string msg;
};
ValidationResult validate_input_grid(const Solver& solver, const InputGrid& input_grid);


/*
 * Functions to test an output grid against a set of input contraints
 *
 * - is_solution: returns true if the output grid is a solution of the set of constraints
 * - list_incompatible_lines: returns the list of all incompatible constraints
 *
 * NB: Will throw on an invalid InputGrid (i.e. not passing check_input_grid)
 * NB: Will throw if the input and output grids' size do not match
 * NB: list_incompatible_lines will throw on a partial output (one with Tile::UNKNWON tiles)
 */
bool is_solution(const InputGrid& input_grid, const OutputGrid& output_grid);
std::vector<LineId> list_incompatible_lines(const InputGrid& input_grid, const OutputGrid& output_grid);


/*
 * Utility functions to build an input constraint from a fully defined Line,
 * or an InputGrid from a fully defined OutputGrid (no Tile::UNKNOWN)
 */
InputGrid::Constraint get_constraint_from(const Line& line);
InputGrid get_input_grid_from(const OutputGrid& grid);

} // namespace picross
