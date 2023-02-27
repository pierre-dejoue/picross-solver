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


std::ostream& operator<<(std::ostream& out, Solver::Status status)
{
    switch (status)
    {
    case Solver::Status::OK:
        out << "OK";
        break;
    case Solver::Status::ABORTED:
        out << "ABORTED";
        break;
    case Solver::Status::CONTRADICTORY_GRID:
        out << "CONTRADICTORY_GRID";
        break;
    case Solver::Status::NOT_LINE_SOLVABLE:
        out << "NOT_LINE_SOLVABLE";
        break;
    default:
        assert(0);  // Unknown Solver::Status
    }
    return out;
}


std::ostream& operator<<(std::ostream& out, ObserverEvent event)
{
    switch (event)
    {
    case ObserverEvent::BRANCHING:
        out << "BRANCHING";
        break;
    case ObserverEvent::KNOWN_LINE:
        out << "KNOWN_LINE";
        break;
    case ObserverEvent::DELTA_LINE:
        out << "DELTA_LINE";
        break;
    case ObserverEvent::SOLVED_GRID:
        out << "SOLVED_GRID";
        break;
    case ObserverEvent::INTERNAL_STATE:
        out << "INTERNAL_STATE";
        break;
    case ObserverEvent::PROGRESS:
        out << "PROGRESS";
        break;
    default:
        assert(0);  // Unknown ObserverEvent
    }
    return out;
}

std::string str_solver_internal_state(unsigned int internal_state)
{
    std::stringstream ss;
    ss << static_cast<WorkGridState>(internal_state);
    return ss.str();
}

ValidationResult validate_input_grid(const Solver& solver, const InputGrid& input_grid, unsigned int max_nb_solutions)
{
    // If max_nb_solutions == 0: No limit is placed on the number of solutions
    if (max_nb_solutions == 1)
    {
        throw std::invalid_argument("Invalid max_nb_solutions: Need to look for at least 2 solutions to test for uniqueness.");
    }

    ValidationResult result;
    result.validation_code = -1;        // ERR
    result.difficulty_code = 0;         // NOT_APPLICABLE
    result.branching_depth = 0u;
    result.msg = "";

    // Check grid
    const auto [check, check_msg] = picross::check_input_grid(input_grid);
    if (!check)
    {
        result.validation_code = -1;
        result.msg = check_msg;
        return result;
    }

    // Solve puzzle
    const auto solver_results = solver.solve(input_grid, max_nb_solutions);

    // Set the validation code
    switch (solver_results.status)
    {
    case Solver::Status::OK:
        result.validation_code = static_cast<ValidationCode>(solver_results.solutions.size());      // code = nb of solutions
        assert(result.validation_code > 0);
        break;

    case Solver::Status::NOT_LINE_SOLVABLE:
        assert(solver_results.solutions.size() == 1);
        assert(solver_results.solutions[0].partial == true);
        assert(solver_results.solutions[0].grid.is_completed() == false);
        assert(solver_results.solutions[0].branching_depth == 0u);
        result.validation_code = 0;                 // No solution
        result.msg = "Not line solvable";
        return result;

    case Solver::Status::ABORTED:
        result.validation_code = -1;
        result.msg = "The solver was aborted";      // TODO Validation with timeout
        return result;

    case Solver::Status::CONTRADICTORY_GRID:
        assert(solver_results.solutions.empty());
        result.validation_code = 0;
        result.msg = "Contradictory grid";
        return result;

    default:
        assert(0);
        result.validation_code = -1;
        result.msg = "Unknown Solver::Status";
        return result;
    }
    assert(solver_results.status == Solver::Status::OK);

    // Check that all the returned solutions are compatible with the input constraints
    const bool solutions_are_compatible = std::all_of(solver_results.solutions.cbegin(), solver_results.solutions.cend(),
        [&input_grid](const auto& solution) { return is_solution(input_grid, solution.grid); });
    if (!solutions_are_compatible)
    {
        result.validation_code = -1;
        result.msg = "Solver ERROR: At least one of the returned solution is incompatible with the input constraints";
        return result;
    }

    // We've checked all the potential errors. The validation code is therefore >= 0
    assert(!solver_results.solutions.empty());
    assert(result.validation_code >= 0);
    assert(std::none_of(solver_results.solutions.cbegin(), solver_results.solutions.cend(), [](const auto& s) { return s.partial; }));
    if (solver_results.solutions.empty())
    {
        assert(result.validation_code == 0);
        assert(result.difficulty_code == 0);
        result.msg = "No solution";             // Did the solver just give up finding a solution?
    }
    else
    {
        assert(result.validation_code > 0);
        const auto minimal_branching_depth_solution_it = std::min_element(solver_results.solutions.cbegin(), solver_results.solutions.cend(),
                                                                          [](const auto& lhs, const auto& rhs) { return lhs.branching_depth < rhs.branching_depth; });
        assert(minimal_branching_depth_solution_it != solver_results.solutions.cend());
        assert(solver_results.solutions.size() == 1 || minimal_branching_depth_solution_it->branching_depth > 0u);   // If there are multiple solutions, then the resolution required branching
        result.branching_depth = minimal_branching_depth_solution_it->branching_depth;
        result.difficulty_code = difficulty_code(solver_results.solutions.size(), result.branching_depth);
        assert(result.difficulty_code > 0);
    }

    return result;
}


DifficultyCode difficulty_code(std::size_t nb_solutions, unsigned int min_branching_depth)
{
    if (nb_solutions == 0u || (nb_solutions > 1u && min_branching_depth == 0u))
    {
        assert(!(nb_solutions > 1u && min_branching_depth == 0u));
        return 0;   // NOT_APPLICABLE
    }
    return (nb_solutions == 1u ? (min_branching_depth == 0u ? 1 : 2) : 3);
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


std::string_view str_difficulty_code(DifficultyCode code)
{
    assert(0 <= code && code <= 3);
    if (code == 1)
    {
        return "LINE";
    }
    else if (code == 2)
    {
        return "BRANCH";
    }
    else if (code == 3)
    {
        return "MULT";
    }
    else
    {
        return "NOT_APPLICABLE";
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
