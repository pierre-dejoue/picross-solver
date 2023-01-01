/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2022 Pierre DEJOUE
 ******************************************************************************/
#include "solver.h"

#include <picross/picross.h>

#include "work_grid.h"
#include "solver_policy.h"

#include <cassert>
#include <exception>
#include <memory>
#include <ostream>
#include <string>
#include <utility>


namespace picross
{

template <bool BranchingAllowed>
Solver::Result RefSolver<BranchingAllowed>::solve(const InputGrid& grid_input, unsigned int max_nb_solutions) const
{
    Result result;

    if (m_stats != nullptr)
    {
        /* Reset stats */
        GridStats new_stats;
        std::swap(*m_stats, new_stats);
        m_stats->max_nb_solutions = max_nb_solutions;
    }

    SolverPolicy_RampUpMaxNbAlternatives solver_policy;
    solver_policy.m_branching_allowed = BranchingAllowed;
    solver_policy.m_limit_on_max_nb_alternatives = BranchingAllowed;

    result.status = Status::OK;
    try
    {
        WorkGrid<SolverPolicy_RampUpMaxNbAlternatives> work_grid(grid_input, solver_policy, m_observer, m_abort_function);
        work_grid.set_stats(m_stats);
        result.status = work_grid.solve(result.solutions, max_nb_solutions);
    }
    catch (const PicrossSolverAborted&)
    {
        result.status = Status::ABORTED;
    }

    return result;
}


template <bool BranchingAllowed>
void RefSolver<BranchingAllowed>::set_observer(Observer observer)
{
    this->m_observer = std::move(observer);
}


template <bool BranchingAllowed>
void RefSolver<BranchingAllowed>::set_stats(GridStats& stats)
{
    this->m_stats = &stats;
}


template <bool BranchingAllowed>
void RefSolver<BranchingAllowed>::set_abort_function(Abort abort)
{
    this->m_abort_function = std::move(abort);
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
    case Solver::Status::NOT_LINE_SOLVABLE:
        ostream << "NOT_LINE_SOLVABLE";
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
    case Solver::Event::INTERNAL_STATE:
        ostream << "INTERNAL_STATE";
        break;
    default:
        assert(0);  // Unknown Solver::Event
    }
    return ostream;
}


ValidationResult validate_input_grid(const Solver& solver, const InputGrid& grid_input)
{
    ValidationResult result;

    const auto [check, check_msg] = picross::check_input_grid(grid_input);

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


std::string_view str_validation_code(ValidationCode code)
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
    return std::make_unique<RefSolver<true>>();
}


std::unique_ptr<Solver> get_line_solver()
{
    return std::make_unique<RefSolver<false>>();
}

} // namespace picross
