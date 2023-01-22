#pragma once

#include <picross/picross.h>

#include <ostream>


namespace picross
{
    const InputGrid::Constraints& get_constraints(const InputGrid& input_grid, Line::Type type);
    void stream_input_grid_constraint(std::ostream& out, const InputGrid& input_grid, const LineId& line_id);
    void stream_input_grid_constraints(std::ostream& out, const InputGrid& input_grid);
} // namespace picross
