#include <picross/picross.h>
#include "picross_stats_internal.h"

#include "binomial.h"

#include <algorithm>
#include <cassert>
#include <cstddef>


namespace picross
{

DifficultyCode difficulty_code(const GridStats& grid_stats)
{
    return difficulty_code(grid_stats.nb_solutions, grid_stats.max_branching_depth);
}

void merge_branching_grid_stats(GridStats& stats, const GridStats& branching_stats)
{
    stats.nb_solutions += branching_stats.nb_solutions;
    stats.max_branching_depth = std::max(stats.max_branching_depth, branching_stats.max_branching_depth);
    stats.nb_branching_calls += branching_stats.nb_branching_calls;
    stats.total_nb_branching_alternatives += branching_stats.total_nb_branching_alternatives;
    stats.nb_probing_calls += branching_stats.nb_probing_calls;
    stats.total_nb_probing_alternatives += branching_stats.total_nb_probing_alternatives;

    stats.max_nb_alternatives_by_branching_depth.resize(stats.max_branching_depth, 0u);
    for (std::size_t d = 0; d < branching_stats.max_nb_alternatives_by_branching_depth.size(); d++)
    {
        assert(d < stats.max_nb_alternatives_by_branching_depth.size());
        stats.max_nb_alternatives_by_branching_depth[d] = std::max(stats.max_nb_alternatives_by_branching_depth[d], branching_stats.max_nb_alternatives_by_branching_depth[d]);
    }

    stats.max_initial_nb_alternatives = std::max(stats.max_initial_nb_alternatives, branching_stats.max_initial_nb_alternatives);
    stats.max_nb_alternatives_partial = std::max(stats.max_nb_alternatives_partial, branching_stats.max_nb_alternatives_partial);
    stats.max_nb_alternatives_partial_w_change = std::max(stats.max_nb_alternatives_partial_w_change, branching_stats.max_nb_alternatives_partial_w_change);
    stats.max_nb_alternatives_linear = std::max(stats.max_nb_alternatives_linear, branching_stats.max_nb_alternatives_linear);
    stats.max_nb_alternatives_linear_w_change = std::max(stats.max_nb_alternatives_linear_w_change, branching_stats.max_nb_alternatives_linear_w_change);
    stats.max_nb_alternatives_full = std::max(stats.max_nb_alternatives_full, branching_stats.max_nb_alternatives_full);
    stats.max_nb_alternatives_full_w_change = std::max(stats.max_nb_alternatives_full_w_change, branching_stats.max_nb_alternatives_full_w_change);
    stats.nb_reduce_list_of_lines_calls += branching_stats.nb_reduce_list_of_lines_calls;
    stats.max_reduce_list_size = std::max(stats.max_reduce_list_size, branching_stats.max_reduce_list_size);
    stats.total_lines_reduced += branching_stats.total_lines_reduced;
    stats.nb_full_grid_pass += branching_stats.nb_full_grid_pass;
    stats.nb_single_line_partial_reduction += branching_stats.nb_single_line_partial_reduction;
    stats.nb_single_line_partial_reduction_w_change += branching_stats.nb_single_line_partial_reduction_w_change;
    stats.nb_single_line_linear_reduction += branching_stats.nb_single_line_linear_reduction;
    stats.nb_single_line_linear_reduction_w_change += branching_stats.nb_single_line_linear_reduction_w_change;
    stats.nb_single_line_full_reduction += branching_stats.nb_single_line_full_reduction;
    stats.nb_single_line_full_reduction_w_change += branching_stats.nb_single_line_full_reduction_w_change;
}

std::ostream& operator<<(std::ostream& out, const GridStats& stats)
{
    out << "  Difficulty: " << str_difficulty_code(difficulty_code(stats)) << std::endl;
    out << "  Number of solutions found: " << stats.nb_solutions << std::endl;
    out << "  Max K: " << stats.max_k << std::endl;
    out << "  Max branching depth: " << stats.max_branching_depth << std::endl;

    if (stats.max_branching_depth > 0u)
    {
        out << "    > Hypothesis (probing/branching) on " << stats.nb_probing_calls << "/" << stats.nb_branching_calls << " lines" << std::endl;
        out << "    > Total number of alternatives being tested (probing/branching): " << stats.total_nb_probing_alternatives << "/" << stats.total_nb_branching_alternatives << std::endl;
        assert(stats.max_nb_alternatives_by_branching_depth.size() == stats.max_branching_depth);
        out << "    > Max number of alternatives by branching depth:";
        for (const auto& max_alternatives : stats.max_nb_alternatives_by_branching_depth)
        {
            out << " " << max_alternatives;
        }
        out << std::endl;
    }

    out << "  Max number of alternatives on an empty line (initial grid pass): " << stats.max_initial_nb_alternatives;
    if (stats.max_initial_nb_alternatives == BinomialCoefficients::overflowValue()) { out << " (overflow!)"; }
    out << std::endl;

    if (stats.max_nb_alternatives_partial_w_change > 0 || stats.max_nb_alternatives_partial)
    {
        out << "  Max number of alternatives after a partial line reduction (change/all): " << stats.max_nb_alternatives_partial_w_change << "/" << stats.max_nb_alternatives_partial << std::endl;
    }
    if (stats.max_nb_alternatives_linear_w_change > 0 || stats.max_nb_alternatives_linear)
    {
        out << "  Max number of alternatives after a  linear line reduction (change/all): " << stats.max_nb_alternatives_linear_w_change << "/" << stats.max_nb_alternatives_linear << std::endl;
    }
    if (stats.max_nb_alternatives_full_w_change > 0 || stats.max_nb_alternatives_full)
    {
        out << "  Max number of alternatives after a    full line reduction (change/all): " << stats.max_nb_alternatives_full_w_change << "/" << stats.max_nb_alternatives_full << std::endl;
    }

    out << "  Number of full grid pass: " << stats.nb_full_grid_pass << std::endl;

    if (stats.nb_single_line_partial_reduction_w_change > 0 || stats.nb_single_line_partial_reduction > 0)
    {
        out << "  Number of single line partial reduction (change/all): " << stats.nb_single_line_partial_reduction_w_change << "/" << stats.nb_single_line_partial_reduction << std::endl;
    }
    if (stats.nb_single_line_linear_reduction_w_change > 0 || stats.nb_single_line_linear_reduction > 0)
    {
        out << "  Number of single line  linear reduction (change/all): " << stats.nb_single_line_linear_reduction_w_change << "/" << stats.nb_single_line_linear_reduction << std::endl;
    }
    if (stats.nb_single_line_full_reduction_w_change > 0 || stats.nb_single_line_full_reduction > 0)
    {
        out << "  Number of single line    full reduction (change/all): " << stats.nb_single_line_full_reduction_w_change << "/" << stats.nb_single_line_full_reduction << std::endl;
    }

    return out;
}

} // namespace picross
