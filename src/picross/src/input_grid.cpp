#include <picross/picross_input_grid.h>

#include "line_constraint.h"

#include <sstream>


namespace picross
{

InputGrid::InputGrid(const Constraints& rows, const Constraints& cols, const std::string_view name)
    : m_rows(rows)
    , m_cols(cols)
    , m_name(name)
    , m_metadata()
{}

InputGrid::InputGrid(Constraints&& rows, Constraints&& cols, const std::string_view name)
    : m_rows(std::move(rows))
    , m_cols(std::move(cols))
    , m_name(name)
    , m_metadata()
{}

void InputGrid::set_name(const std::string_view name)
{
    m_name = std::string(name);
}

void InputGrid::set_metadata(std::string_view key, std::string_view data)
{
    m_metadata.insert_or_assign(std::string(key), std::string(data));
}

std::string str_input_grid_size(const InputGrid& grid)
{
    std::stringstream ss;
    ss << grid.cols().size() << "x" << grid.rows().size();
    return ss.str();
}

std::pair<bool, std::string> check_input_grid(const InputGrid& grid)
{
    // Sanity check of the grid input
    //  -> height != 0 and width != 0
    //  -> same number of painted cells on rows and columns
    //  -> for each constraint min_line_size is smaller than the width or height of the row or column, respectively.

    const auto width  = static_cast<unsigned int>(grid.width());
    const auto height = static_cast<unsigned int>(grid.height());

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
    for (const auto& c : grid.rows())
    {
        LineConstraint row(Line::ROW, c);
        nb_tiles_on_rows += row.nb_filled_tiles();
        if (row.min_line_size() > width)
        {
            std::ostringstream oss;
            oss << "Width = " << width << " of the grid is too small for constraint: " << row;
            return std::make_pair(false, oss.str());
        }
    }
    unsigned int nb_tiles_on_cols = 0u;
    for (const auto& c : grid.cols())
    {
        LineConstraint col(Line::COL, c);
        nb_tiles_on_cols += col.nb_filled_tiles();
        if (col.min_line_size() > height)
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

} // namespace picross