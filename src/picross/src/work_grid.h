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
#include "line_cache.h"
#include "line_constraint.h"

#include <picross/picross.h>

#include <stdutils/macros.h>

#include <memory>
#include <ostream>
#include <vector>

namespace picross {

/*
 * State of the grid solver
 */
enum class WorkGridState
{
    INITIAL_PASS,
    LINEAR_REDUCTION,
    FULL_REDUCTION,
    PROBING,
    BRANCHING,
    STOP_SOLVER
};

std::ostream& operator<<(std::ostream& out, WorkGridState state);

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
    using AllLines = std::vector<LineId>;
private:
    struct ProbingResult
    {
        Solver::Status  m_status            = Solver::Status::OK;
        bool            m_grid_has_changed  = false;
        bool            m_continue_probing  = false;
    };
public:
    WorkGrid(const InputGrid& grid, const SolverPolicy& solver_policy, Observer observer = Observer(), Solver::Abort abort_function = Solver::Abort(), float min_progress = 0.f, float max_progress = 1.f);
    // Not movable
    WorkGrid(WorkGrid&&) noexcept = delete;
    WorkGrid& operator=(WorkGrid&&) noexcept = delete;
private:
    WorkGrid(const WorkGrid& parent);                 // Allocate a nested search grid, in a reset state
    WorkGrid& operator=(const WorkGrid& parent);      // Copy the grid data, some of the main data structures, and reset others
public:
    void set_stats(GridStats* stats);
    Solver::Status line_solve(const Solver::SolutionFound& solution_found);
    Solver::Status solve(const Solver::SolutionFound& solution_found);
private:
    WorkGrid<SolverPolicy>& nested_work_grid();
    void configure(const SolverPolicy& solver_policy, WorkGridState initial_state, GridStats* stats, float min_progress, float max_progress);
    Solver::Status line_solve(const Solver::SolutionFound& solution_found, bool currently_probing);
    bool all_lines_completed() const;
    bool update_line(const LineSpan& line, unsigned int nb_alt);
    void partition_completed_lines();
    std::vector<LineId> sorted_edges() const;
    std::vector<LineId> sorted_lines_next_to_completed() const;
    void sort_by_nb_alternatives();
    bool is_sorted_by_nb_alternatives() const;
    LineId next_line_for_search() const;
    PassStatus single_line_initial_pass(Line::Type type, unsigned int index);
    PassStatus single_line_linear_reduction(Line::Type type, unsigned int index);
    PassStatus single_line_full_reduction(Line::Type type, unsigned int index);
    template <WorkGridState S>
    PassStatus full_grid_pass();
    ProbingResult probe();
    ProbingResult probe(LineId line_id);
    Solver::Status branch(const Solver::SolutionFound& solution_found);
    bool is_valid_solution() const;
    bool found_solution(const Solver::SolutionFound& solution_found) const;
    void fill_cache_with_orthogonal_lines(LineId line_id);
    void set_orthogonal_lines_from_cache(WorkGrid& target_grid, const LineSpan& alternative) const;
private:
    WorkGridState                                   m_state;
    SolverPolicy                                    m_solver_policy;
    unsigned int                                    m_max_k;             // Maximum nb of segments on a line constraint
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
    Observer                                        m_observer;          // If not empty, the solver will notify the observer of its progress
    Solver::Abort                                   m_abort_function;    // If not empty, the solver will regularly call this function and abort if it returns true
    unsigned int                                    m_max_nb_alternatives;
    unsigned int                                    m_branching_depth;
    unsigned int                                    m_probing_depth_incr;
    std::pair<float, float>                         m_progress_bar;
    std::unique_ptr<WorkGrid<SolverPolicy>>         m_nested_work_grid;
    LineCache                                       m_branch_line_cache;
    std::shared_ptr<FullReductionBuffers>           m_full_reduction_buffers;
    std::shared_ptr<binomial::Cache>                m_binomial;
};

} // namespace picross
