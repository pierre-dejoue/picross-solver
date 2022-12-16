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


Grid::Grid(size_t width, size_t height, const std::string& name)
    : m_width(width)
    , m_height(height)
    , m_name(name)
    , m_grid()
{
    reset();
}

Grid& Grid::operator=(const Grid& other)
{
    const_cast<std::size_t&>(m_width) = other.m_width;
    const_cast<std::size_t&>(m_height) = other.m_height;
    const_cast<std::string&>(m_name) = other.m_name;
    m_grid = other.m_grid;
    return *this;
}

Grid& Grid::operator=(Grid&& other) noexcept
{
    const_cast<std::size_t&>(m_width) = other.m_width;
    const_cast<std::size_t&>(m_height) = other.m_height;
    const_cast<std::string&>(m_name) = std::move(other.m_name);
    m_grid = std::move(other.m_grid);
    return *this;
}

Tile Grid::get(size_t x, size_t y) const
{
    assert(x < m_width);
    assert(y < m_height);
    return m_grid[x * m_height + y];
}

template <>
Line Grid::get_line<Line::ROW>(size_t index) const
{
    assert(index < m_height);
    std::vector<Tile> tiles;
    tiles.reserve(m_width);
    for (unsigned int x = 0u; x < m_width; x++)
    {
        tiles.push_back(get(x, index));
    }
    return Line(Line::ROW, index, std::move(tiles));
}

template <>
Line Grid::get_line<Line::COL>(size_t index) const
{
    assert(index < m_width);
    std::vector<Tile> tiles;
    tiles.reserve(m_height);
    tiles.insert(tiles.cbegin(), m_grid.data() + index * m_height, m_grid.data() + (index + 1) * m_height);
    return Line(Line::COL, index, std::move(tiles));
}

Line Grid::get_line(Line::Type type, size_t index) const
{
    return type == Line::ROW ? get_line<Line::ROW>(index) : get_line<Line::COL>(index);
}

bool Grid::set(size_t x, size_t y, Tile val)
{
    assert(x < m_width);
    assert(y < m_height);
    return Tiles::set(m_grid[x * m_height + y], val);
}

void Grid::reset()
{
    auto empty_grid = std::vector<Tile>(m_height * m_width, Tile::UNKNOWN);
    std::swap(m_grid, empty_grid);
}

bool Grid::is_solved() const
{
    return std::none_of(std::cbegin(m_grid), std::cend(m_grid), [](const Tile& t) { return t == Tile::UNKNOWN; });
}

std::size_t Grid::hash() const
{
    // Intentionally ignore the name to compute the hash
    std::size_t result = 0u;
    result ^= std::hash<std::remove_const_t<decltype(m_width)>>{}(m_width);
    result ^= std::hash<std::remove_const_t<decltype(m_height)>>{}(m_height);

    // Encode the grid contents with pairs of booleans
    std::vector<bool> grid_bool;
    grid_bool.reserve(2 * m_grid.size());
    for (const Tile tile : m_grid)
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
    assert(grid_bool.size() == 2 * m_grid.size());
    assert(grid_bool.size() == 2 * m_width * m_height);

    result ^= std::hash<decltype(grid_bool)>{}(grid_bool);

    return result;
}

bool operator==(const Grid& lhs, const Grid& rhs)
{
    // Intentionally ignore the name
    return lhs.m_width == rhs.m_width
        && lhs.m_height == rhs.m_height
        && lhs.m_grid == rhs.m_grid;
}

bool operator!=(const Grid& lhs, const Grid& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& ostream, const Grid& grid)
{
    for (unsigned int y = 0u; y < grid.height(); y++)
    {
        const Line row = grid.get_line<Line::ROW>(y);
        ostream << "  " << str_line(row) << std::endl;
    }
    return ostream;
}

} // namespace picross
