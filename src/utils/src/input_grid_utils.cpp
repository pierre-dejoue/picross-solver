#include <utils/input_grid_utils.h>

#include <algorithm>
#include <cassert>


namespace picross
{
    void stream_input_grid_constraint(std::ostream& out, const InputGrid& input_grid, const LineId& line_id)
    {
        const auto& constraint = get_constraint(input_grid, line_id);
        if (constraint.empty())
        {
            out << '0';
        }
        else
        {
            out << *constraint.cbegin();
            std::for_each(std::next(constraint.cbegin()), constraint.cend(), [&out](const auto c) { out << ' ' << c; });
        }
    }

    void stream_input_grid_line_id_and_constraint(std::ostream& out, const InputGrid& input_grid, const LineId& line_id)
    {
        out << "line: " << line_id;
        out << " segments: ";
        stream_input_grid_constraint(out, input_grid, line_id);
        out << std::endl;
    }

    void stream_input_grid_constraints(std::ostream& out, const InputGrid& input_grid)
    {
        for (auto y = 0u; y < input_grid.height(); y++)
        {
            out << "CONSTRAINT ";
            stream_input_grid_line_id_and_constraint(out, input_grid, LineId(Line::ROW, y));
        }
        for (auto x = 0u; x < input_grid.width(); x++)
        {
            out << "CONSTRAINT ";
            stream_input_grid_line_id_and_constraint(out, input_grid, LineId(Line::COL, x));
        }
    }

} // namespace picross
