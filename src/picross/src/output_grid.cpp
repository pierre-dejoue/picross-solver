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


const std::string& OutputGrid::get_name() const
{
    return name;
}


void OutputGrid::set_name(const std::string& name)
{
    this->name = name;
}


Tile::Type OutputGrid::get(size_t x, size_t y) const
{
    if (x >= width) { throw std::out_of_range("OutputGrid::set: x (" + std::to_string(x) + ") is out of range (" + std::to_string(width) + ")"); }
    if (y >= height) { throw std::out_of_range("OutputGrid::set: y (" + std::to_string(y) + ") is out of range (" + std::to_string(height) + ")"); }
    return grid[x * height + y];
}


Line OutputGrid::get_line(Line::Type type, size_t index) const
{
    std::vector<Tile::Type> tiles;
    if (type == Line::ROW)
    {
        if (index >= height) { throw std::out_of_range("OutputGrid::get_line: row index (" + std::to_string(index) + ") is out of range (" + std::to_string(height) + ")"); }
        tiles.reserve(width);
        for (unsigned int x = 0u; x < width; x++)
        {
            tiles.push_back(get(x, index));
        }
    }
    else
    {
        assert(type == Line::COL);
        if (index >= width) { throw std::out_of_range("OutputGrid::get_line: column index (" + std::to_string(index) + ") is out of range (" + std::to_string(width) + ")"); }
        tiles.reserve(height);
        tiles.insert(tiles.cbegin(), grid.data() + index * height, grid.data() + (index + 1) * height);
    }
    return Line(type, index, std::move(tiles));
}


void OutputGrid::set(size_t x, size_t y, Tile::Type t)
{
    if (x >= width) { throw std::out_of_range("OutputGrid::set: x (" + std::to_string(x) + ") is out of range (" + std::to_string(width) + ")"); }
    if (y >= height) { throw std::out_of_range("OutputGrid::set: y (" + std::to_string(y) + ") is out of range (" + std::to_string(height) + ")"); }
    grid[x * height + y] = t;
}


void OutputGrid::reset()
{
    auto empty_grid = std::vector<Tile::Type>(height * width, Tile::UNKNOWN);
    std::swap(grid, empty_grid);
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
