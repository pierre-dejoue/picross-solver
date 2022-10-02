/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2022 Pierre DEJOUE
 ******************************************************************************/
#include "solver.h"

#include <picross/picross.h>

#include "work_grid.h"

#include <cassert>
#include <exception>
#include <memory>
#include <ostream>
#include <string>
#include <utility>


namespace picross
{

Solver::Result RefSolver::solve(const InputGrid& grid_input, unsigned int max_nb_solutions) const
{
    Result result;

    if (stats != nullptr)
    {
        /* Reset stats */
        GridStats new_stats;
        std::swap(*stats, new_stats);
        stats->max_nb_solutions = max_nb_solutions;
    }

    Observer observer_wrapper;
    if (observer)
    {
        observer_wrapper = [this](Solver::Event event, const Line* delta, unsigned int depth)
        {
            if (this->stats != nullptr)
            {
                this->stats->nb_observer_callback_calls++;
            }
            this->observer(event, delta, depth);
        };
    }

    result.status = Status::OK;
    try
    {
        WorkGrid<LineSelectionPolicy_RampUpMaxNbAlternatives_EstimateNbAlternatives> work_grid(grid_input, std::move(observer_wrapper), abort_function);
        work_grid.set_stats(stats);
        result.status = work_grid.solve(result.solutions, max_nb_solutions);
    }
    catch (const PicrossSolverAborted&)
    {
        result.status = Status::ABORTED;
    }

    return result;
}


void RefSolver::set_observer(Observer observer)
{
    this->observer = std::move(observer);
}


void RefSolver::set_stats(GridStats& stats)
{
    this->stats = &stats;
}

void RefSolver::set_abort_function(Abort abort)
{
    this->abort_function = std::move(abort);
}


std::ostream& operator<<(std::ostream& ostream, Solver::Status status)
{
    switch (status)
    {
    case Solver::Status::OK:
        ostream << "OK";
        break;
    case Solver::Status::ABORTED:
        ostream << "ABORTED";
        break;
    case Solver::Status::CONTRADICTORY_GRID:
        ostream << "CONTRADICTORY_GRID";
        break;
    default:
        assert(0);  // Unknown Solver::Status
    }
    return ostream;
}


std::ostream& operator<<(std::ostream& ostream, Solver::Event event)
{
    switch (event)
    {
    case Solver::Event::BRANCHING:
        ostream << "BRANCHING";
        break;
    case Solver::Event::DELTA_LINE:
        ostream << "DELTA_LINE";
        break;
    case Solver::Event::SOLVED_GRID:
        ostream << "SOLVED_GRID";
        break;
    default:
        assert(0);  // Unknown Solver::Event
    }
    return ostream;
}


ValidationResult validate_input_grid(const Solver& solver, const InputGrid& grid_input)
{
    ValidationResult result;

    const auto [check, check_msg] = picross::check_grid_input(grid_input);

    if (!check)
    {
        result.code = -1;
        result.branching_depth = 0;
        result.msg = check_msg;
        return result;
    }

    const auto solver_results = solver.solve(grid_input, 2);

    if (solver_results.status == Solver::Status::CONTRADICTORY_GRID)
    {
        result.code = -1;
        result.branching_depth = 0;
        result.msg = "Input grid constraints are contradictory";
        return result;
    }

    assert(solver_results.status == Solver::Status::OK);
    result.code = static_cast<ValidationCode>(solver_results.solutions.size());
    result.branching_depth = 0;
    result.msg = "";
    if (solver_results.solutions.empty())
    {
        assert(result.code == 0);
        result.msg = "No solution could be found";
    }
    else if (solver_results.solutions.size() == 1)
    {
        assert(result.code == 1);
        result.branching_depth = solver_results.solutions[0].branching_depth;
    }
    else
    {
        result.code = 2;
        result.branching_depth = solver_results.solutions[0].branching_depth;
        assert(result.branching_depth != 0);    // If there are multiple solutions the grid is not line solvable
        result.msg = "The solution is not unique";
    }
    return result;
}


std::string_view validation_code_str(ValidationCode code)
{
    if (code < 0)
    {
        return "ERR";
    }
    else if (code == 0)
    {
        return "ZERO";
    }
    else if(code == 1)
    {
        return "OK";
    }
    else
    {
        assert(code > 1);
        return "MULT";
    }
}


std::unique_ptr<Solver> get_ref_solver()
{
    return std::make_unique<RefSolver>();
}


} // namespace picross
