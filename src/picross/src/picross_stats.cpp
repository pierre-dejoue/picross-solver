#include <picross/picross_stats.h>
#include "picross_stats_internal.h"

#include "binomial.h"

#include <algorithm>
#include <cassert>
#include <cstddef>


namespace picross
{

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
    stats.max_nb_alternatives_full = std::max(stats.max_nb_alternatives_full, branching_stats.max_nb_alternatives_full);
    stats.max_nb_alternatives_full_w_change = std::max(stats.max_nb_alternatives_full_w_change, branching_stats.max_nb_alternatives_full_w_change);
    stats.nb_reduce_list_of_lines_calls += branching_stats.nb_reduce_list_of_lines_calls;
    stats.max_reduce_list_size = std::max(stats.max_reduce_list_size, branching_stats.max_reduce_list_size);
    stats.total_lines_reduced += branching_stats.total_lines_reduced;
    stats.nb_full_grid_pass += branching_stats.nb_full_grid_pass;
    stats.nb_single_line_partial_reduction += branching_stats.nb_single_line_partial_reduction;
    stats.nb_single_line_partial_reduction_w_change += branching_stats.nb_single_line_partial_reduction_w_change;
    stats.nb_single_line_full_reduction += branching_stats.nb_single_line_full_reduction;
    stats.nb_single_line_full_reduction_w_change += branching_stats.nb_single_line_full_reduction_w_change;
}

std::ostream& operator<<(std::ostream& ostream, const GridStats& stats)
{
    ostream << "  Number of solutions found: " << stats.nb_solutions << std::endl;
    ostream << "  Max branching depth: " << stats.max_branching_depth << std::endl;
    if (stats.max_branching_depth > 0u)
    {
        ostream << "    > Hypothesis (probing/branching) on " << stats.nb_probing_calls << "/" << stats.nb_branching_calls << " lines" << std::endl;
        ostream << "    > Total number of alternatives being tested (probing/branching): " << stats.total_nb_probing_alternatives << "/" << stats.total_nb_branching_alternatives << std::endl;
        assert(stats.max_nb_alternatives_by_branching_depth.size() == stats.max_branching_depth);
        ostream << "    > Max number of alternatives by branching depth:";
        for (const auto& max_alternatives : stats.max_nb_alternatives_by_branching_depth)
        {
            ostream << " " << max_alternatives;
        }
        ostream << std::endl;
    }
    ostream << "  Max number of alternatives on an empty line (initial grid pass): " << stats.max_initial_nb_alternatives;
    if (stats.max_initial_nb_alternatives == BinomialCoefficients::overflowValue())
    {
        ostream << " (overflow!)";
    }
    ostream << std::endl;
    ostream << "  Max number of alternatives after a partial line reduction (change/all): " << stats.max_nb_alternatives_partial_w_change << "/" << stats.max_nb_alternatives_partial << std::endl;
    ostream << "  Max number of alternatives after a    full line reduction (change/all): " << stats.max_nb_alternatives_full_w_change << "/" << stats.max_nb_alternatives_full << std::endl;
    ostream << "  Number of full grid pass: " << stats.nb_full_grid_pass << std::endl;
    ostream << "  Number of single line partial reduction (change/all): " << stats.nb_single_line_partial_reduction_w_change << "/" << stats.nb_single_line_partial_reduction << std::endl;
    ostream << "  Number of single line    full reduction (change/all): " << stats.nb_single_line_full_reduction_w_change << "/" << stats.nb_single_line_full_reduction << std::endl;

    return ostream;
}

} // namespace picross
