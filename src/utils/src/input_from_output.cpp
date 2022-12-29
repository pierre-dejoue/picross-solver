#include <utils/input_from_output.h>

#include <cassert>

namespace picross
{

InputGrid build_input_grid_from(const OutputGrid& grid)
{
    InputGrid result;

    result.m_name = grid.name();

    result.m_rows.reserve(grid.height());
    for (unsigned int y = 0u; y < grid.height(); y++)
    {
        result.m_rows.emplace_back(get_constraint_from(grid.get_line<Line::ROW>(y)));
    }

    result.m_cols.reserve(grid.width());
    for (unsigned int x = 0u; x < grid.width(); x++)
    {
        result.m_cols.emplace_back(get_constraint_from(grid.get_line<Line::COL>(x)));
    }

    assert(result.width() == grid.width());
    assert(result.height() == grid.height());
    return result;
}

} // namespace picross
