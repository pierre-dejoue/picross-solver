/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2022 Pierre DEJOUE
 ******************************************************************************/
#include "solver.h"

#include <picross/picross.h>

#include "work_grid.h"
#include "solver_policy.h"

#include <algorithm>
#include <cassert>
#include <exception>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>


namespace picross
{

template <bool BranchingAllowed>
Solver::Result RefSolver<BranchingAllowed>::solve(const InputGrid& input_grid, unsigned int max_nb_solutions) const
{
    Result result;

    if (m_stats != nullptr)
    {
        /* Reset stats */
        GridStats new_stats;
        std::swap(*m_stats, new_stats);
    }

    SolverPolicy_RampUpMaxNbAlternatives solver_policy;
    solver_policy.m_branching_allowed = BranchingAllowed;
    solver_policy.m_limit_on_max_nb_alternatives = false;

    WorkGrid<SolverPolicy_RampUpMaxNbAlternatives> work_grid(input_grid, solver_policy, m_observer, m_abort_function);
    work_grid.set_stats(m_stats);

    SolutionFound solution_found = [&result, max_nb_solutions](Solution&& solution) -> bool
    {
        result.solutions.emplace_back(std::move(solution));
        return max_nb_solutions == 0 || result.solutions.size() < max_nb_solutions;
    };
    result.status = work_grid.solve(solution_found);
    if (result.status == Solver::Status::ABORTED && result.solutions.size() == max_nb_solutions)
        result.status = Solver::Status::OK;

    return result;
}

template <bool BranchingAllowed>
Solver::Status RefSolver<BranchingAllowed>::solve(const InputGrid& input_grid, SolutionFound solution_found) const
{
    if (m_stats != nullptr)
    {
        /* Reset stats */
        GridStats new_stats;
        std::swap(*m_stats, new_stats);
    }

    SolverPolicy_RampUpMaxNbAlternatives solver_policy;
    solver_policy.m_branching_allowed = BranchingAllowed;
    solver_policy.m_limit_on_max_nb_alternatives = false;

    WorkGrid<SolverPolicy_RampUpMaxNbAlternatives> work_grid(input_grid, solver_policy, m_observer, m_abort_function);
    work_grid.set_stats(m_stats);

    return work_grid.solve(solution_found);
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


std::ostream& operator<<(std::ostream& ostream, ObserverEvent event)
{
    switch (event)
    {
    case ObserverEvent::BRANCHING:
        ostream << "BRANCHING";
        break;
    case ObserverEvent::KNOWN_LINE:
        ostream << "KNOWN_LINE";
        break;
    case ObserverEvent::DELTA_LINE:
        ostream << "DELTA_LINE";
        break;
    case ObserverEvent::SOLVED_GRID:
        ostream << "SOLVED_GRID";
        break;
    case ObserverEvent::INTERNAL_STATE:
        ostream << "INTERNAL_STATE";
        break;
    case ObserverEvent::PROGRESS:
        ostream << "PROGRESS";
        break;
    default:
        assert(0);  // Unknown ObserverEvent
    }
    return ostream;
}

std::string str_solver_internal_state(unsigned int internal_state)
{
    std::stringstream ss;
    ss << static_cast<WorkGridState>(internal_state);
    return ss.str();
}

ValidationResult validate_input_grid(const Solver& solver, const InputGrid& input_grid)
{
    ValidationResult result;

    const auto [check, check_msg] = picross::check_input_grid(input_grid);

    if (!check)
    {
        result.code = -1;
        result.branching_depth = 0;
        result.msg = check_msg;
        return result;
    }

    const auto solver_results = solver.solve(input_grid, 2);

    switch (solver_results.status)
    {
    case Solver::Status::OK:
        result.code = static_cast<ValidationCode>(solver_results.solutions.size());     // Temporary initialization value
        result.branching_depth = 0;
        result.msg = "";
        break;

    case Solver::Status::NOT_LINE_SOLVABLE:
        assert(solver_results.solutions.size() == 1);
        assert(solver_results.solutions[0].partial == true);
        assert(solver_results.solutions[0].grid.is_completed() == false);
        assert(solver_results.solutions[0].branching_depth == 0);
        result.code = 0;
        result.branching_depth = 0;
        result.msg = "Not line solvable";
        return result;

    case Solver::Status::ABORTED:
        result.code = -1;
        result.branching_depth = 0;
        result.msg = "The solver was aborted";      // TODO Validation with timeout
        return result;

    case Solver::Status::CONTRADICTORY_GRID:
        result.code = 0;
        result.branching_depth = 0;
        result.msg = "No solution";
        return result;

    default:
        assert(0);
        result.code = -1;
        result.branching_depth = 0;
        result.msg = "Unknown error";
        return result;
    }
    assert(solver_results.status == Solver::Status::OK);

    // Check that all the returned solutions are compatible with the input constraints
    const bool solutions_are_compatible = std::all_of(solver_results.solutions.cbegin(), solver_results.solutions.cend(),
        [&input_grid](const auto& solution) { return is_solution(input_grid, solution.grid); });
    if (!solutions_are_compatible)
    {
        result.code = -1;
        result.branching_depth = 0;
        result.msg = "Solver error: at least one of the returned solution is incompatible with the input constraints";
    }

    // We've checked all the potential errors. The validation code is therefore >= 0
    assert(!solver_results.solutions.empty());
    if (solver_results.solutions.empty())
    {
        result.code = 0;
        result.msg = "No solution";
    }
    else if (solver_results.solutions.size() == 1)
    {
        assert(result.code == 1);
        result.branching_depth = solver_results.solutions[0].branching_depth;
    }
    else
    {
        const auto minimal_branching_depth_solution_it = std::min_element(solver_results.solutions.cbegin(), solver_results.solutions.cend(),
                                                                          [](const auto& lhs, const auto& rhs) { return lhs.branching_depth < rhs.branching_depth; });
        assert(minimal_branching_depth_solution_it != solver_results.solutions.cend());
        assert(minimal_branching_depth_solution_it->branching_depth > 0);   // If there are multiple solutions the resolution required branching
        result.code = 2;
        result.branching_depth = minimal_branching_depth_solution_it->branching_depth;
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
