/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PRIVATE API of the Picross solver
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include "binomial.h"

#include "line_constraint.h"

#include <picross/picross.h>

#include <algorithm>
#include <exception>
#include <limits>
#include <memory>
#include <vector>


namespace picross
{

class PicrossGridCannotBeSolved : public std::runtime_error
{
public:
    PicrossGridCannotBeSolved() : std::runtime_error("[PicrossSolver] The current grid cannot be solved")
    {}
};


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
        return std::numeric_limits<unsigned int>::max();
    }

    // Return true if it is time to move to branching exploration
    static bool switch_to_branching(unsigned int max_nb_alternatives, bool grid_changed, unsigned int skipped_lines, unsigned int search_depth)
    {
        return !grid_changed;
    }
};

struct LineSelectionPolicy_RampUpNbAlternatives
{
    static constexpr unsigned int min_nb_alternatives = 1 << 6;
    static constexpr unsigned int max_nb_alternatives = 1 << 30;

    static constexpr unsigned int initial_max_nb_alternatives()
    {
        return min_nb_alternatives;
    }

    static unsigned int get_max_nb_alternatives(unsigned int previous_max_nb_alternatives, bool grid_changed, unsigned int skipped_lines, unsigned int search_depth)
    {
        unsigned int nb_alternatives = previous_max_nb_alternatives;
        if (grid_changed && previous_max_nb_alternatives > min_nb_alternatives)
        {
            // Decrease max_nb_alternatives
            nb_alternatives = std::min(nb_alternatives, max_nb_alternatives) >> 2;
        }
        else if (!grid_changed && skipped_lines > 0u)
        {
            // Increase max_nb_alternatives
            nb_alternatives = nb_alternatives >= max_nb_alternatives
                ? std::numeric_limits<unsigned int>::max()
                : nb_alternatives << 2;
        }
        return nb_alternatives;
    }

    static bool switch_to_branching(unsigned int max_nb_alternatives, bool grid_changed, unsigned int skipped_lines, unsigned int search_depth)
    {
        return !grid_changed && skipped_lines == 0u;
    }
};


/*
 * WorkGrid class
 *
 *   Working class used to solve a grid.
 */
template <typename LineSelectionPolicy>
class WorkGrid final : public OutputGrid
{
public:
    WorkGrid(const InputGrid& grid, Solver::Solutions* solutions, GridStats* stats = nullptr, Solver::Observer observer = Solver::Observer());
    WorkGrid(const WorkGrid& other) = delete;
    WorkGrid& operator=(const WorkGrid& other) = delete;
    WorkGrid(WorkGrid&& other) = default;
    WorkGrid& operator=(WorkGrid&& other) = default;
private:
    WorkGrid(const WorkGrid& parent, unsigned int nested_level);
public:
    bool solve(unsigned int max_nb_solutions = 0u);
private:
    bool all_lines_completed() const;
    bool set_w_reduce_flag(size_t x, size_t y, Tile::Type t);
    bool set_line(const Line& line);
    bool single_line_initial_pass(Line::Type type, unsigned int index);
    bool single_line_pass(Line::Type type, unsigned int index);
    bool full_side_pass(Line::Type type, unsigned int& skipped_lines, bool first_pass = false);
    bool full_grid_pass(unsigned int& skipped_lines, bool first_pass = false);
    bool guess(unsigned int max_nb_solutions) const;
    bool valid_solution() const;
    void save_solution() const;
private:
    std::vector<LineConstraint>                 rows;
    std::vector<LineConstraint>                 cols;
    Solver::Solutions*                          saved_solutions; // ptr to a vector where to store solutions
    GridStats*                                  stats;           // if not null, the solver will store some stats in that structure
    Solver::Observer                            observer;        // if not empty, the solver will notify the observer of its progress
    std::vector<bool>                           line_completed[2];
    std::vector<bool>                           line_to_be_reduced[2];
    std::vector<unsigned int>                   nb_alternatives[2];
    unsigned int                                max_nb_alternatives;
    std::vector<Line>                           guess_list_of_all_alternatives;
    unsigned int                                nested_level;    // nested_level is incremented by function Grid::guess()
    std::unique_ptr<BinomialCoefficientsCache>  binomial;
};


} // namespace picross
