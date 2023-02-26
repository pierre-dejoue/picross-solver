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
        const bool changed = (val != t);
        t = val;
        return changed;
    }

    inline bool update(Tile& t, Tile val)
    {
        if (t != Tile::UNKNOWN)
        {
            assert((val == Tile::UNKNOWN) || (val == t));
            return false;
        }
        const bool changed = (val != Tile::UNKNOWN);
        t = val;
        return changed;
    }
} // namespace Tiles
} // namespace


Grid::Grid(std::size_t width, std::size_t height, std::string_view name)
    : m_width(width)
    , m_height(height)
    , m_name(name)
    , m_row_major()
    , m_col_major()
{
    m_row_major.resize(width * height, Tile::UNKNOWN);
    m_col_major.resize(width * height, Tile::UNKNOWN);
}

Grid& Grid::operator=(const Grid& other)
{
    assert(m_width == other.m_width);
    assert(m_height == other.m_height);
    m_row_major = other.m_row_major;
    m_col_major = other.m_col_major;
    return *this;
}

Grid& Grid::operator=(Grid&& other) noexcept
{
    assert(m_width == other.m_width);
    assert(m_height == other.m_height);
    m_row_major = std::move(other.m_row_major);
    m_col_major = std::move(other.m_col_major);
    return *this;
}


const Grid::Container& Grid::get_container(Line::Type type) const
{
    return type == Line::ROW ? m_row_major : m_col_major;
}

LineSpan Grid::get_line(Line::Type type, Line::Index index) const
{
    return const_cast<Grid&>(*this).get_line<LineSpan>(type, index);
}

LineSpan Grid::get_line(LineId line_id) const
{
    return const_cast<Grid&>(*this).get_line<LineSpan>(line_id.m_type, line_id.m_index);
}

template <typename LineSpanT>
LineSpanT Grid::get_line(Line::Type type, Line::Index index)
{
    assert((type == Line::ROW && index < m_height) || (type == Line::COL && index < m_width));
    auto& tiles = type == Line::ROW ? m_row_major : m_col_major;
    const auto line_length = type == Line::ROW ? m_width : m_height;
    return LineSpanT(type, index, line_length, &tiles[index * line_length]);
}

Tile Grid::get(Line::Index x, Line::Index y) const
{
    assert(x < m_width);
    assert(y < m_height);
    return get_line(Line::COL, x)[static_cast<int>(y)];
}

bool Grid::set(Line::Index x, Line::Index y, Tile val)
{
    assert(x < m_width);
    assert(y < m_height);
    Tiles::set(m_row_major[y * m_width + x], val);
    return Tiles::set(m_col_major[x * m_height + y], val);
}

bool Grid::update(Line::Index x, Line::Index y, Tile val)
{
    assert(x < m_width);
    assert(y < m_height);
    Tiles::update(m_row_major[y * m_width + x], val);
    return Tiles::update(m_col_major[x * m_height + y], val);
}

void Grid::reset()
{
    for (std::size_t idx = 0; idx < m_width * m_height; idx++)
    {
        m_row_major[idx] = m_col_major[idx] = Tile::UNKNOWN;
    }
}

bool Grid::is_completed() const
{
    return std::all_of(std::cbegin(m_row_major), std::cend(m_row_major), [](const Tile& t) { return t != Tile::UNKNOWN; });
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
    for (const Tile tile : m_row_major)
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
        && lhs.m_row_major == rhs.m_row_major;
}

bool operator!=(const Grid& lhs, const Grid& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& out, const Grid& grid)
{
    for (unsigned int y = 0u; y < grid.height(); y++)
        out << grid.get_line(Line::ROW, y) << std::endl;
    return out;
}


template <Line::Type T>
GridSnapshot<T>::GridSnapshot(std::size_t width, std::size_t height)
    : m_width(width)
    , m_height(height)
    , m_tiles()
{
    m_tiles.resize(width * height, Tile::UNKNOWN);
}

template <Line::Type T>
GridSnapshot<T>::GridSnapshot(const Grid& grid)
    : m_width(grid.width())
    , m_height(grid.height())
    , m_tiles(grid.get_container(T))
{
}

template <Line::Type T>
GridSnapshot<T>& GridSnapshot<T>::operator=(const GridSnapshot<T>& other)
{
    const_cast<std::size_t&>(m_width) = other.m_width;
    const_cast<std::size_t&>(m_height) = other.m_height;
    m_tiles = other.m_tiles;
    return *this;
}
template <Line::Type T>
GridSnapshot<T>& GridSnapshot<T>::operator=(GridSnapshot<T>&& other) noexcept
{
    const_cast<std::size_t&>(m_width) = other.m_width;
    const_cast<std::size_t&>(m_height) = other.m_height;
    m_tiles = std::move(other.m_tiles);
    return *this;
}

template <Line::Type T>
GridSnapshot<T>& GridSnapshot<T>::operator=(const Grid& grid)
{
    const_cast<std::size_t&>(m_width) = grid.width();
    const_cast<std::size_t&>(m_height) = grid.height();
    m_tiles = grid.get_container(T);
    return *this;
}

template <Line::Type T>
LineSpan GridSnapshot<T>::get_line(Line::Index index) const
{
    assert((T == Line::ROW && index < m_height) || (T == Line::COL && index < m_width));
    const auto line_length = T == Line::ROW ? m_width : m_height;
    return LineSpan(T, index, line_length, &m_tiles[index * line_length]);
}

template <Line::Type T>
void GridSnapshot<T>::reduce(const Grid& grid)
{
    const Grid::Container& other_tiles = grid.get_container(T);
    std::size_t idx {0};
    for (Tile& tile : m_tiles)
    {
        if (tile != Tile::UNKNOWN && tile != other_tiles[idx])
            tile = Tile::UNKNOWN;
        idx++;
    }
}

// Explicit template instantiation
template LineSpan  Grid::get_line<LineSpan>(Line::Type, Line::Index);
template LineSpanW Grid::get_line<LineSpanW>(Line::Type, Line::Index);

template class GridSnapshot<Line::ROW>;

} // namespace picross
