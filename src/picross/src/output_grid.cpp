/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#include <picross/picross.h>

#include "line.h"

#include <algorithm>
#include <cassert>
#include <utility>


namespace picross
{

OutputGrid::OutputGrid(size_t width, size_t height, const std::string& name) :
    m_width(width),
    m_height(height),
    m_name(name),
    m_grid()
{
    reset();
}

OutputGrid& OutputGrid::operator=(const OutputGrid& other)
{
    const_cast<std::size_t&>(m_width) = other.m_width;
    const_cast<std::size_t&>(m_height) = other.m_height;
    m_name = other.m_name;
    m_grid = other.m_grid;
    return *this;
}

OutputGrid& OutputGrid::operator=(OutputGrid&& other) noexcept
{
    const_cast<std::size_t&>(m_width) = other.m_width;
    const_cast<std::size_t&>(m_height) = other.m_height;
    m_name = std::move(other.m_name);
    m_grid = std::move(other.m_grid);
    return *this;
}

void OutputGrid::set_name(const std::string& name)
{
    this->m_name = name;
}


Tile::Type OutputGrid::get(size_t x, size_t y) const
{
    if (x >= m_width) { throw std::out_of_range("OutputGrid::set: x (" + std::to_string(x) + ") is out of range (" + std::to_string(m_width) + ")"); }
    if (y >= m_height) { throw std::out_of_range("OutputGrid::set: y (" + std::to_string(y) + ") is out of range (" + std::to_string(m_height) + ")"); }
    return m_grid[x * m_height + y];
}


template <>
Line OutputGrid::get_line<Line::ROW>(size_t index) const
{
    if (index >= m_height) { throw std::out_of_range("OutputGrid::get_line: row index (" + std::to_string(index) + ") is out of range (" + std::to_string(m_height) + ")"); }
    std::vector<Tile::Type> tiles;
    tiles.reserve(m_width);
    for (unsigned int x = 0u; x < m_width; x++)
    {
        tiles.push_back(get(x, index));
    }
    return Line(Line::ROW, index, std::move(tiles));
}


template <>
Line OutputGrid::get_line<Line::COL>(size_t index) const
{
    if (index >= m_width) { throw std::out_of_range("OutputGrid::get_line: column index (" + std::to_string(index) + ") is out of range (" + std::to_string(m_width) + ")"); }
    std::vector<Tile::Type> tiles;
    tiles.reserve(m_height);
    tiles.insert(tiles.cbegin(), m_grid.data() + index * m_height, m_grid.data() + (index + 1) * m_height);
    return Line(Line::COL, index, std::move(tiles));
}


// Explicit template instantiations
template Line OutputGrid::get_line<Line::ROW>(size_t index) const;
template Line OutputGrid::get_line<Line::COL>(size_t index) const;

Line OutputGrid::get_line(Line::Type type, size_t index) const
{
    return type == Line::ROW ? get_line<Line::ROW>(index) : get_line<Line::COL>(index);
}


bool OutputGrid::set(size_t x, size_t y, Tile::Type val)
{
    if (x >= m_width) { throw std::out_of_range("OutputGrid::set: x (" + std::to_string(x) + ") is out of range (" + std::to_string(m_width) + ")"); }
    if (y >= m_height) { throw std::out_of_range("OutputGrid::set: y (" + std::to_string(y) + ") is out of range (" + std::to_string(m_height) + ")"); }
    return Tile::set(m_grid[x * m_height + y], val);
}


void OutputGrid::reset()
{
    auto empty_grid = std::vector<Tile::Type>(m_height * m_width, Tile::UNKNOWN);
    std::swap(m_grid, empty_grid);
}


bool OutputGrid::is_solved() const
{
    return std::none_of(std::cbegin(m_grid), std::cend(m_grid), [](const Tile::Type& t) { return t == Tile::UNKNOWN; });
}


bool operator==(const OutputGrid& lhs, const OutputGrid& rhs)
{
    return lhs.m_width == rhs.m_width
        && lhs.m_height == rhs.m_height
        && lhs.m_grid == rhs.m_grid;
}


bool operator!=(const OutputGrid& lhs, const OutputGrid& rhs)
{
    return !(lhs == rhs);
}


std::ostream& operator<<(std::ostream& ostream, const OutputGrid& grid)
{
    for (unsigned int y = 0u; y < grid.height(); y++)
    {
        const Line row = grid.get_line<Line::ROW>(y);
        ostream << "  " << str_line(row) << std::endl;
    }
    return ostream;
}


} // namespace picross
