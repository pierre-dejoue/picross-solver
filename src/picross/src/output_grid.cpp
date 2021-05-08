/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#include <picross/picross.h>

#include "line.h"

#include <algorithm>
#include <cassert>


namespace picross
{

OutputGrid::OutputGrid(size_t width, size_t height, const std::string& name) :
    width(width),
    height(height),
    name(name),
    grid()
{
    reset();
}


OutputGrid& OutputGrid::operator=(const OutputGrid& other)
{
    assert(width == other.width);
    assert(height == other.height);
    assert(name == other.name);
    grid = other.grid;
    return *this;
}


OutputGrid& OutputGrid::operator=(OutputGrid&& other)
{
    assert(width == other.width);
    assert(height == other.height);
    assert(name == other.name);
    grid = std::move(other.grid);
    return *this;
}


const std::string& OutputGrid::get_name() const
{
    return name;
}


size_t OutputGrid::get_width() const
{
    return width;
}


size_t OutputGrid::get_height() const
{
    return height;
}


Tile::Type OutputGrid::get(size_t x, size_t y) const
{
    if (x >= width) { throw std::invalid_argument("OutputGrid::set: x is out of range"); }
    if (y >= height) { throw std::invalid_argument("OutputGrid::set: y is out of range"); }
    return grid[x * height + y];
}


void OutputGrid::set(size_t x, size_t y, Tile::Type t)
{
    if (x >= width) { throw std::invalid_argument("OutputGrid::set: x is out of range"); }
    if (y >= height) { throw std::invalid_argument("OutputGrid::set: y is out of range"); }
    grid[x * height + y] = t;
}


void OutputGrid::reset()
{
    auto empty_grid = std::vector<Tile::Type>(height * width, Tile::UNKNOWN);
    std::swap(grid, empty_grid);
}


Line OutputGrid::get_line(Line::Type type, size_t index) const
{
    std::vector<Tile::Type> tiles;
    if (type == Line::ROW)
    {
        if (index >= height) { throw std::invalid_argument("Line ctor: row index is out of range"); }
        tiles.reserve(width);
        for (unsigned int x = 0u; x < width; x++)
        {
            tiles.push_back(get(x, index));
        }
    }
    else
    {
        assert(type == Line::COL);
        if (index >= width) { throw std::invalid_argument("Line ctor: column index is out of range"); }
        tiles.reserve(height);
        tiles.insert(tiles.cbegin(), grid.data() + index * height, grid.data() + (index + 1) * height);
    }
    return Line(type, index, std::move(tiles));
}


bool OutputGrid::is_solved() const
{
    return std::none_of(grid.cbegin(), grid.cend(), [](const Tile::Type& t) { return t == Tile::UNKNOWN; });
}


std::ostream& operator<<(std::ostream& ostream, const OutputGrid& grid)
{
    for (unsigned int y = 0u; y < grid.get_height(); y++)
    {
        const Line row = grid.get_line(Line::ROW, y);
        ostream << "  " << str_line(row) << std::endl;
    }
    return ostream;
}


} // namespace picross
