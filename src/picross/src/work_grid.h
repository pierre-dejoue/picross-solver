/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PRIVATE API of the Picross solver
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include "binomial.h"
#include "grid.h"
#include "line.h"
#include "line_alternatives.h"
#include "line_constraint.h"
#include "macros.h"

#include <picross/picross.h>

#include <memory>
#include <vector>

namespace picross
{

/*
 * WorkGrid class
 *
 *   Working class used to solve a grid.
 */
template <typename SolverPolicy>
class WorkGrid final : private Grid
{
private:
    struct PassStatus
    {
        bool grid_changed = false;
        bool contradictory = false;
        unsigned int skipped_lines = 0u;

        PassStatus& operator+=(const PassStatus& other)
        {
            grid_changed |= other.grid_changed;
            contradictory |= other.contradictory;
            skipped_lines += other.skipped_lines;
            return *this;
        }
    };
    enum class State
    {
        INITIAL_PASS,
        PARTIAL_REDUCTION,
        LINEAR_REDUCTION,
        FULL_REDUCTION,
        PROBING,
        BRANCHING
    };
    using AllLines = std::vector<LineId>;
private:
    struct ProbingResult
    {
        Solver::Status  m_status            = Solver::Status::OK;
        bool            m_grid_has_changed  = false;
    };
public:
    WorkGrid(const InputGrid& grid, const SolverPolicy& solver_policy, Solver::Observer observer = Solver::Observer(), Solver::Abort abort_function = Solver::Abort());
    // Not copyable nor movable
    WorkGrid(const WorkGrid&) = delete;
    WorkGrid(WorkGrid&&) noexcept = delete;
    WorkGrid& operator=(const WorkGrid&) = delete;
    WorkGrid& operator=(WorkGrid&&) noexcept = delete;
private:
    // Allocate nested work grid
    WorkGrid(const WorkGrid& parent, const SolverPolicy& solver_policy, State initial_state);
public:
    void set_stats(GridStats* stats);
    Solver::Status line_solve(const Solver::SolutionFound& solution_found);
    Solver::Status solve(const Solver::SolutionFound& solution_found);
private:
    Solver::Status line_solve(const Solver::SolutionFound& solution_found, bool probing);
    bool all_lines_completed() const;
    bool update_line(const LineSpan& line, unsigned int nb_alt);
    void partition_completed_lines();
    std::vector<LineId> sorted_edges() const;
    std::vector<LineId> sorted_lines_next_to_completed() const;
    void sort_by_nb_alternatives();
    PassStatus single_line_initial_pass(Line::Type type, unsigned int index);
    PassStatus single_line_partial_reduction(Line::Type type, unsigned int index);
    PassStatus single_line_linear_reduction(Line::Type type, unsigned int index);
    PassStatus single_line_full_reduction(Line::Type type, unsigned int index);
    template <State S>
    PassStatus full_grid_pass();
    ProbingResult probe();
    ProbingResult probe(LineId line_id);
    Solver::Status branch(const Solver::SolutionFound& solution_found);
    bool is_valid_solution() const;
    bool found_solution(const Solver::SolutionFound& solution_found) const;
private:
    State                                           m_state;
    const SolverPolicy                              m_solver_policy;
    std::vector<LineConstraint>                     m_constraints[2];
    std::vector<LineAlternatives>                   m_alternatives[2];
    std::vector<bool>                               m_line_completed[2];
    std::vector<bool>                               m_line_has_updates[2];
    std::vector<bool>                               m_line_is_fully_reduced[2];
    std::vector<bool>                               m_line_probed[2];
    std::vector<unsigned int>                       m_nb_alternatives[2];
    LineRange                                       m_uncompleted_lines_range[2];
    AllLines                                        m_all_lines;
    AllLines::iterator                              m_uncompleted_lines_end;
    GridStats*                                      m_grid_stats;        // If not null, the solver will store some stats in that structure
    Solver::Observer                                m_observer;          // If not empty, the solver will notify the observer of its progress
    Solver::Abort                                   m_abort_function;    // If not empty, the solver will regularly call this function and abort if it returns true
    unsigned int                                    m_max_nb_alternatives;
    unsigned int                                    m_branching_depth;
    unsigned int                                    m_probing_depth_incr;
    std::shared_ptr<BinomialCoefficients::Cache>    m_binomial;
};

} // namespace picross
