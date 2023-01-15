/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PUBLIC API of the Picross solver
 *
 *   - Solver stats
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include <ostream>
#include <vector>


namespace picross
{

/*
 * GridStats
 *
 *   A data structure to store some stats related to the grid and the solver.
 */
struct GridStats
{
    unsigned int nb_solutions = 0u;
    unsigned int max_branching_depth = 0u;                              // max branching depth of the solver (not of the solutions)
    unsigned int nb_branching_calls = 0u;
    unsigned int total_nb_branching_alternatives = 0u;
    unsigned int nb_probing_calls = 0u;
    unsigned int total_nb_probing_alternatives = 0u;
    unsigned int max_initial_nb_alternatives = 0u;
    unsigned int max_nb_alternatives = 0u;
    unsigned int max_nb_alternatives_w_change = 0u;
    unsigned int nb_reduce_list_of_lines_calls = 0u;
    unsigned int max_reduce_list_size = 0u;
    unsigned int total_lines_reduced = 0u;
    unsigned int nb_full_grid_pass = 0u;
    unsigned int nb_single_line_partial_reduction = 0u;
    unsigned int nb_single_line_partial_reduction_w_change = 0u;
    unsigned int nb_single_line_full_reduction = 0u;
    unsigned int nb_single_line_full_reduction_w_change = 0u;
    std::vector<unsigned int> max_nb_alternatives_by_branching_depth;   // vector with max_branching_depth elements
};

std::ostream& operator<<(std::ostream& ostream, const GridStats& stats);

} // namespace picross
