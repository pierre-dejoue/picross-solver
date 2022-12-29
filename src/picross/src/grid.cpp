#include "grid.h"
#include "line.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <type_traits>
#include <utility>


namespace picross
{

namespace
{
namespace Tiles
{
    inline bool set(Tile& t, Tile val)
    {
        if (val != Tile::UNKNOWN && t != val)
        {
            assert(t == Tile::UNKNOWN);
            t = val;
            return true;
        }
        return false;
    }
} // namespace Tiles
} // namespace


Grid::Grid(std::size_t width, std::size_t height, const std::string& name)
    : m_width(width)
    , m_height(height)
    , m_name(name)
    , m_rows()
    , m_cols()
{
    m_rows.reserve(height);
    for (Line::Index row_idx = 0; row_idx < height; row_idx++)
    {
        m_rows.emplace_back(Line::ROW, row_idx, width, Tile::UNKNOWN);
    }
    m_cols.reserve(width);
    for (Line::Index col_idx = 0; col_idx < width; col_idx++)
    {
        m_cols.emplace_back(Line::COL, col_idx, height, Tile::UNKNOWN);
    }
}

Grid& Grid::operator=(const Grid& other)
{
    const_cast<std::size_t&>(m_width) = other.m_width;
    const_cast<std::size_t&>(m_height) = other.m_height;
    const_cast<std::string&>(m_name) = other.m_name;
    m_rows = other.m_rows;
    m_cols = other.m_cols;
    return *this;
}

Grid& Grid::operator=(Grid&& other) noexcept
{
    const_cast<std::size_t&>(m_width) = other.m_width;
    const_cast<std::size_t&>(m_height) = other.m_height;
    const_cast<std::string&>(m_name) = std::move(other.m_name);
    m_rows = std::move(other.m_rows);
    m_cols = std::move(other.m_cols);
    return *this;
}

Tile Grid::get(Line::Index x, Line::Index y) const
{
    assert(x < m_width);
    assert(y < m_height);
    assert(m_rows[y].tiles()[x] == m_cols[x].tiles()[y]);
    return m_rows[y].tiles()[x];
}

template <>
const Line& Grid::get_line<Line::ROW>(Line::Index index) const
{
    assert(index < m_height);
    return m_rows[index];
}

template <>
const Line& Grid::get_line<Line::COL>(Line::Index index) const
{
    assert(index < m_width);
    return m_cols[index];
}

const Line& Grid::get_line(Line::Type type, Line::Index index) const
{
    return type == Line::ROW ? get_line<Line::ROW>(index) : get_line<Line::COL>(index);
}

bool Grid::set(Line::Index x, Line::Index y, Tile val)
{
    assert(x < m_width);
    assert(y < m_height);
    Tiles::set(m_rows[y].tiles()[x], val);
    return Tiles::set(m_cols[x].tiles()[y], val);
}

void Grid::reset()
{
    for (std::size_t row_idx = 0; row_idx < m_height; row_idx++)
    for (std::size_t col_idx = 0; col_idx < m_width; col_idx++)
    {
        m_rows[row_idx].tiles()[col_idx] = m_cols[col_idx].tiles()[row_idx] = Tile::UNKNOWN;
    }
}

bool Grid::is_solved() const
{
    return std::all_of(std::cbegin(m_rows), std::cend(m_rows), [](const Line& line) {
        return std::none_of(std::cbegin(line.tiles()), std::cend(line.tiles()), [](const Tile& t) { return t == Tile::UNKNOWN; });
    });
}

std::size_t Grid::hash() const
{
    // Intentionally ignore the name to compute the hash
    std::size_t result = 0u;
    result ^= std::hash<std::remove_const_t<decltype(m_width)>>{}(m_width);
    result ^= std::hash<std::remove_const_t<decltype(m_height)>>{}(m_height);

    // Encode the grid contents with pairs of booleans
    std::vector<bool> grid_bool;
    grid_bool.reserve(2 * m_width * m_height);
    for (const Line& row : m_rows)
        for (const Tile tile : row.tiles())
        {
            if (tile != Tile::UNKNOWN)
            {
                grid_bool.push_back(true);
                grid_bool.push_back(tile != Tile::EMPTY);
            }
            else
            {
                grid_bool.push_back(false);
                grid_bool.push_back(false);
            }
        }
    assert(grid_bool.size() == 2 * m_width * m_height);

    result ^= std::hash<decltype(grid_bool)>{}(grid_bool);

    return result;
}

bool operator==(const Grid& lhs, const Grid& rhs)
{
    // Intentionally ignore the name
    return lhs.m_width == rhs.m_width
        && lhs.m_height == rhs.m_height
        && lhs.m_rows == rhs.m_rows;
}

bool operator!=(const Grid& lhs, const Grid& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& ostream, const Grid& grid)
{
    for (unsigned int y = 0u; y < grid.height(); y++)
        ostream << grid.get_line<Line::ROW>(y) << std::endl;
    return ostream;
}

} // namespace picross
