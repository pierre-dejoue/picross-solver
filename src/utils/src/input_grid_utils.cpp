#include <utils/input_grid_utils.h>

#include <cassert>


namespace picross
{

    const InputGrid::Constraints& get_constraints(const InputGrid& input_grid, Line::Type type)
    {
        if (type == Line::ROW)
        {
            return input_grid.m_rows;
        }
        else
        {
            assert(type == Line::COL);
            return input_grid.m_cols;
        }
    }

    void stream_input_grid_constraint(std::ostream& out, const InputGrid& input_grid, const LineId& line_id)
    {
        out << "CONSTRAINT line: " << str_line_id(line_id) << " segments:";
        const auto& constraint = get_constraints(input_grid, line_id.m_type).at(line_id.m_index);
        if (constraint.empty())
        {
            out << " 0";
        }
        else
        {
            for (const auto& c: constraint)
                out << " " << c;
        }
        out << std::endl;
    }

    void stream_input_grid_constraints(std::ostream& out, const InputGrid& input_grid)
    {
        for (auto y = 0; y < input_grid.height(); y++)
            stream_input_grid_constraint(out, input_grid, LineId(Line::ROW, y));
        for (auto x = 0; x < input_grid.width(); x++)
            stream_input_grid_constraint(out, input_grid, LineId(Line::COL, x));
    }

} // namespace picross
