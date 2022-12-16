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
#include "line_alternatives.h"
#include "line_constraint.h"
#include "macros.h"

#include <picross/picross.h>

#include <algorithm>
#include <exception>
#include <limits>
#include <memory>
#include <vector>


namespace picross
{

/*
 * Policies that affect how the solver select which lines to reduce
 */
struct LineSelectionPolicy_Legacy
{
    // Solver initial value of max_nb_alternatives
    static constexpr unsigned int initial_max_nb_alternatives()
    {
        return std::numeric_limits<unsigned int>::max();
    }

    // get the value for max_nb_alternatives to be used on the next full grid pass
    static unsigned int get_max_nb_alternatives(unsigned int previous_max_nb_alternatives, bool grid_changed, unsigned int skipped_lines, unsigned int search_depth)
    {
        UNUSED(previous_max_nb_alternatives);
        UNUSED(grid_changed);
        UNUSED(skipped_lines);
        UNUSED(search_depth);
        return std::numeric_limits<unsigned int>::max();
    }

    // estimate the number of alternatives of a line after a new tile is set
    static void estimate_nb_alternatives(unsigned int& nb_alternatives)
    {
        UNUSED(nb_alternatives);
        // do not change
    }

    // Return true if it is time to move to branching exploration
    static bool switch_to_branching(unsigned int max_nb_alternatives, bool grid_changed, unsigned int skipped_lines, unsigned int search_depth)
    {
        UNUSED(max_nb_alternatives);
        UNUSED(skipped_lines);
        UNUSED(search_depth);
        return !grid_changed;
    }
};

struct LineSelectionPolicy_RampUpMaxNbAlternatives
{
    static inline constexpr unsigned int Min_nb_alternatives = 1 << 6;
    static inline constexpr unsigned int Max_nb_alternatives = 1 << 30;

    static constexpr unsigned int initial_max_nb_alternatives()
    {
        return Min_nb_alternatives;
    }

    static unsigned int get_max_nb_alternatives(unsigned int previous_max_nb_alternatives, bool grid_changed, unsigned int skipped_lines, unsigned int search_depth)
    {
        UNUSED(search_depth);
        unsigned int nb_alternatives = previous_max_nb_alternatives;
        if (grid_changed && previous_max_nb_alternatives > Min_nb_alternatives)
        {
            // Decrease max_nb_alternatives
            nb_alternatives = std::min(nb_alternatives, Max_nb_alternatives) >> 2;
        }
        else if (!grid_changed && skipped_lines > 0u)
        {
            // Increase max_nb_alternatives
            nb_alternatives = nb_alternatives >= Max_nb_alternatives
                ? std::numeric_limits<unsigned int>::max()
                : nb_alternatives << 2;
        }
        return nb_alternatives;
    }

    // estimate the number of alternatives of a line after a new tile is set
    static void estimate_nb_alternatives(unsigned int& nb_alternatives)
    {
        UNUSED(nb_alternatives);
        // do not change
    }

    static bool switch_to_branching(unsigned int max_nb_alternatives, bool grid_changed, unsigned int skipped_lines, unsigned int search_depth)
    {
        UNUSED(max_nb_alternatives);
        UNUSED(search_depth);
        return !grid_changed && skipped_lines == 0u;
    }
};

struct LineSelectionPolicy_RampUpMaxNbAlternatives_EstimateNbAlternatives :
    public LineSelectionPolicy_RampUpMaxNbAlternatives
{
    // estimate the number of alternatives of a line after a new tile is set
    static void estimate_nb_alternatives(unsigned int& nb_alternatives)
    {
        nb_alternatives = std::max(2u, nb_alternatives >> 1);
    }
};


// Exception returned by WorkGrid::solve() if the processing was aborted from the outside
class PicrossSolverAborted : public std::exception
{};


/*
 * WorkGrid class
 *
 *   Working class used to solve a grid.
 */
template <typename LineSelectionPolicy, bool BranchingAllowed = true>
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
public:
    WorkGrid(const InputGrid& grid, Solver::Observer observer = Solver::Observer(), Solver::Abort abort_function = Solver::Abort());
    // Not copyable nor movable
    WorkGrid(const WorkGrid&) = delete;
    WorkGrid(WorkGrid&&) noexcept = delete;
    WorkGrid& operator=(const WorkGrid&) = delete;
    WorkGrid& operator=(WorkGrid&&) noexcept = delete;
private:
    WorkGrid(const WorkGrid& parent, unsigned int nested_level);            // Shallow copy
public:
    void set_stats(GridStats* stats);
    Solver::Status line_solve(Solver::Solutions& solutions);
    Solver::Status solve(Solver::Solutions& solutions, unsigned int max_nb_solutions = 0u);
private:
    bool all_lines_completed() const;
    bool set_line(const Line& line, unsigned int nb_alt = 0u);
    PassStatus single_line_initial_pass(Line::Type type, unsigned int index);
    PassStatus single_line_pass(Line::Type type, unsigned int index);
    PassStatus full_side_pass(Line::Type type);
    PassStatus full_grid_initial_pass();
    PassStatus full_grid_pass();
    Solver::Status branch(Solver::Solutions& solutions, unsigned int max_nb_solutions) const;
    bool valid_solution() const;
    void save_solution(Solver::Solutions& solutions) const;
private:
    std::vector<LineConstraint>                 m_constraints[2];
    std::vector<LineAlternatives>               m_alternatives[2];
    std::vector<bool>                           m_line_completed[2];
    std::vector<bool>                           m_line_to_be_reduced[2];
    std::vector<unsigned int>                   m_nb_alternatives[2];
    GridStats*                                  m_grid_stats;        // If not null, the solver will store some stats in that structure
    Solver::Observer                            m_observer;          // If not empty, the solver will notify the observer of its progress
    Solver::Abort                               m_abort_function;    // If not empty, the solver will regularly call this function and abort if it returns true
    unsigned int                                m_max_nb_alternatives;
    std::vector<Line>                           m_guess_list_of_all_alternatives;
    unsigned int                                m_branching_depth;
    std::unique_ptr<BinomialCoefficientsCache>  m_binomial;
};


} // namespace picross
