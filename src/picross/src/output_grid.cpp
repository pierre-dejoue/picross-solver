/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#include <picross/picross.h>

#include "grid.h"

#include <cassert>
#include <exception>

namespace picross
{

OutputGrid::OutputGrid(std::size_t width, std::size_t height, const std::string& name)
    : p_grid(std::make_unique<Grid>(width, height, name))
{
    assert(p_grid);
}

OutputGrid::OutputGrid(const Grid& grid)
    : p_grid(std::make_unique<Grid>(grid))
{
    assert(p_grid);
}

OutputGrid::OutputGrid(Grid&& grid)
    : p_grid(std::make_unique<Grid>(std::move(grid)))
{
    assert(p_grid);
}

OutputGrid::~OutputGrid() = default;

OutputGrid::OutputGrid(const OutputGrid& other)
    : p_grid(std::make_unique<Grid>(*other.p_grid))
{
    assert(p_grid);
}

OutputGrid::OutputGrid(OutputGrid&&) noexcept = default;

OutputGrid& OutputGrid::operator=(const OutputGrid& other)
{
    p_grid = std::make_unique<Grid>(*other.p_grid);
    assert(p_grid);
    return *this;
}

OutputGrid& OutputGrid::operator=(OutputGrid&&) noexcept = default;

std::size_t OutputGrid::width() const { return p_grid->width(); }

std::size_t OutputGrid::height() const { return p_grid->height(); }

const std::string& OutputGrid::name() const { return p_grid->name(); }

Tile OutputGrid::get_tile(std::size_t x, std::size_t y) const
{
    if (x >= p_grid->width()) { throw std::out_of_range("OutputGrid::get_tile: x (" + std::to_string(x) + ") is out of range (" + std::to_string(p_grid->width()) + ")"); }
    if (y >= p_grid->height()) { throw std::out_of_range("OutputGrid::get_tile: y (" + std::to_string(y) + ") is out of range (" + std::to_string(p_grid->height()) + ")"); }
    return p_grid->get(x, y);
}

template <Line::Type Type>
Line OutputGrid::get_line(size_t index) const
{
    if constexpr (Type == Line::ROW)
        if (index >= p_grid->height()) { throw std::out_of_range("Grid::get_line: row index (" + std::to_string(index) + ") is out of range (" + std::to_string(p_grid->height()) + ")"); }
    if constexpr (Type == Line::COL)
        if (index >= p_grid->width()) { throw std::out_of_range("Grid::get_line: column index (" + std::to_string(index) + ") is out of range (" + std::to_string(p_grid->width()) + ")"); }
    return p_grid->get_line<Type>(index);
}

Line OutputGrid::get_line(Line::Type type, size_t index) const
{
    return type == Line::ROW ? get_line<Line::ROW>(index) : get_line<Line::COL>(index);
}


bool OutputGrid::is_solved() const
{
    return p_grid->is_solved();
}


std::size_t OutputGrid::hash() const
{
    return p_grid->hash();
}

bool OutputGrid::set_tile(std::size_t x, std::size_t y, Tile val)
{
    if (x >= p_grid->width()) { throw std::out_of_range("Grid::set_tile: x (" + std::to_string(x) + ") is out of range (" + std::to_string(p_grid->width()) + ")"); }
    if (y >= p_grid->height()) { throw std::out_of_range("Grid::set_tile: y (" + std::to_string(y) + ") is out of range (" + std::to_string(p_grid->height()) + ")"); }
    return p_grid->set(x, y, val);
}

void OutputGrid::reset()
{
    p_grid->reset();
}

bool operator==(const OutputGrid& lhs, const OutputGrid& rhs)
{
    // Intentionally ignore the name
    return *lhs.p_grid == *rhs.p_grid;
}

bool operator!=(const OutputGrid& lhs, const OutputGrid& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& ostream, const OutputGrid& grid)
{
    return ostream << *grid.p_grid;
}

} // namespace picross
