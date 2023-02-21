/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#include <picross/picross.h>

#include "line_constraint.h"
#include "line_span.h"
#include "macros.h"

#include <cassert>
#include <exception>
#include <sstream>

namespace picross
{

std::string str_output_grid_size(const OutputGrid& grid)
{
    std::ostringstream oss;
    oss << grid.width() << "x" << grid.height();
    return oss.str();
}

namespace
{
    template <typename NonMatching>
    void match_output_grid_against_input_grid(const InputGrid& input_grid, const OutputGrid& output_grid, const NonMatching& non_matching)
    {
        // We intentionally ignore the grid name

        // Check input grid
        const auto [check, check_msg] = check_input_grid(input_grid);
        if (!check)
        {
            throw std::invalid_argument("Invalid input grid. Error message: " + check_msg);
        }

        // Check that the output grid is complete (no Tile::UNKNOWN)
        if (!output_grid.is_completed())
        {
            throw std::invalid_argument("Incomplete output grid");
        }

        // Check that input and output grid size match
        if (input_grid.width() != output_grid.width() || input_grid.height() != output_grid.height())
        {
            throw std::invalid_argument("The output grid's size (" + str_output_grid_size(output_grid) + ") does not match the input (" + str_input_grid_size(input_grid) + ")");
        }

        // Check that all constraints match
        const auto check_line_constraint = [&](const LineId line_id) {
            if (!LineConstraint(line_id.m_type, get_constraint(input_grid, line_id)).compatible(output_grid.get_line(line_id))) { non_matching(line_id); }
        };
        for (unsigned int x = 0u; x < input_grid.width(); x++)
            check_line_constraint(LineId(Line::COL, x));
        for (unsigned int y = 0u; y < input_grid.height(); y++)
            check_line_constraint(LineId(Line::ROW, y));
    }
}

bool is_solution(const InputGrid& input_grid, const OutputGrid& output_grid)
{
    if (!output_grid.is_completed())
        return false;

    bool result = true;
    const auto non_matching = [&result](const LineId& line_id) { UNUSED(line_id); result = false; };
    match_output_grid_against_input_grid(input_grid, output_grid, non_matching);
    return result;
}

std::vector<LineId> list_incompatible_lines(const InputGrid& input_grid, const OutputGrid& output_grid)
{
    std::vector<LineId> result;
    const auto non_matching = [&result](const LineId& line_id) { result.emplace_back(line_id); };
    match_output_grid_against_input_grid(input_grid, output_grid, non_matching);
    return result;
}

InputGrid get_input_grid_from(const OutputGrid& grid)
{
    assert(grid.is_completed());
    InputGrid::Constraints rows, cols;

    rows.reserve(grid.height());
    for (unsigned int y = 0u; y < grid.height(); y++)
    {
        rows.emplace_back(get_constraint_from(grid.get_line<Line::ROW>(y)));
    }

    cols.reserve(grid.width());
    for (unsigned int x = 0u; x < grid.width(); x++)
    {
        cols.emplace_back(get_constraint_from(grid.get_line<Line::COL>(x)));
    }

    InputGrid result(std::move(rows), std::move(cols), grid.name());
    assert(result.width() == grid.width());
    assert(result.height() == grid.height());
    return result;
}

} // namespace picross
