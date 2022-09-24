/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2022 Pierre DEJOUE
 ******************************************************************************/
#include <picross/picross.h>

#include "line_constraint.h"
#include "picross_solver_version.h"

#include <algorithm>
#include <cassert>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>


namespace picross
{


std::string_view get_version_string()
{
    return std::string_view(PICROSS_SOLVER_VERSION_STRING);
}


std::string grid_size_str(const InputGrid& grid)
{
    std::ostringstream oss;
    oss << grid.cols.size() << "x" << grid.rows.size();
    return oss.str();
}


std::pair<bool, std::string> check_grid_input(const InputGrid& grid)
{
    // Sanity check of the grid input
    //  -> height != 0 and width != 0
    //  -> same number of painted cells on rows and columns
    //  -> for each constraint min_line_size is smaller than the width or height of the row or column, respectively.

    const auto width  = static_cast<unsigned int>(grid.cols.size());
    const auto height = static_cast<unsigned int>(grid.rows.size());

    if (height == 0u)
    {
        std::ostringstream oss;
        oss << "Invalid height = " << height;
        return std::make_pair(false, oss.str());
    }
    if (width == 0u)
    {
        std::ostringstream oss;
        oss << "Invalid width = " << width;
        return std::make_pair(false, oss.str());
    }
    unsigned int nb_tiles_on_rows = 0u;
    for (const auto& c : grid.rows)
    {
        LineConstraint row(Line::ROW, c);
        nb_tiles_on_rows += row.nb_filled_tiles();
        if (row.get_min_line_size() > width)
        {
            std::ostringstream oss;
            oss << "Width = " << width << " of the grid is too small for constraint: " << row;
            return std::make_pair(false, oss.str());
        }
    }
    unsigned int nb_tiles_on_cols = 0u;
    for (const auto& c : grid.cols)
    {
        LineConstraint col(Line::COL, c);
        nb_tiles_on_cols += col.nb_filled_tiles();
        if (col.get_min_line_size() > height)
        {
            std::ostringstream oss;
            oss << "Height = " << height << " of the grid is too small for constraint: " << col;
            return std::make_pair(false, oss.str());
        }
    }
    if (nb_tiles_on_rows != nb_tiles_on_cols)
    {
        std::ostringstream oss;
        oss << "Number of filled tiles on rows (" << nb_tiles_on_rows << ") and columns (" <<  nb_tiles_on_cols << ") do not match";
        return std::make_pair(false, oss.str());
    }
    return std::make_pair(true, std::string());
}


std::ostream& operator<<(std::ostream& ostream, const GridStats& stats)
{
    ostream << "  Number of solutions found: " << stats.nb_solutions;
    if (stats.max_nb_solutions > 0)
    {
        ostream << "/" << stats.max_nb_solutions;
    }
    ostream << std::endl;
    ostream << "  Max branching depth: " << stats.max_nested_level << std::endl;
    if (stats.max_nested_level > 0u)
    {
        ostream << "    > Hypothesis on " << stats.guess_total_calls << " lines" << std::endl;
        ostream << "    > Total number of alternatives being tested: " << stats.guess_total_alternatives << std::endl;
        assert(stats.guess_max_nb_alternatives_by_depth.size() == stats.max_nested_level);
        ostream << "    > Max number of alternatives by branching depth:";
        for (const auto& max_alternatives : stats.guess_max_nb_alternatives_by_depth)
        {
            ostream << " " << max_alternatives;
        }
        ostream << std::endl;
    }
    ostream << "  Max number of alternatives on an empty line (initial grid pass): " << stats.max_initial_nb_alternatives;
    if (stats.max_initial_nb_alternatives == BinomialCoefficientsCache::overflowValue())
    {
        ostream << " (overflow!)";
    }
    ostream << std::endl;
    ostream << "  Max number of alternatives after a line reduce (change/all): " << stats.max_nb_alternatives_w_change << "/" << stats.max_nb_alternatives << std::endl;
    ostream << "  Number of calls to full_grid_pass: " << stats.nb_full_grid_pass_calls << std::endl;
    ostream << "  Number of calls to single_line_pass (change/all): " << stats.nb_single_line_pass_calls_w_change << "/" << stats.nb_single_line_pass_calls << std::endl;
    ostream << "  Number of calls to the observer callback: " << stats.nb_observer_callback_calls << std::endl;

    return ostream;
}


} // namespace picross
