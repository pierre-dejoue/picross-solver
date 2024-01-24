/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#include "line.h"

#include "line_span.h"

#include <algorithm>
#include <cassert>
#include <exception>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace picross {

Line::Line(Line::Type type, Line::Index index, size_t size, Tile init_tile) :
    m_type(type),
    m_index(index),
    m_tiles(size, init_tile)
{
}


Line::Line(Line::Type type, Line::Index index, const Line::Container& tiles) :
    m_type(type),
    m_index(index),
    m_tiles(tiles)
{
}


Line::Line(Line::Type type, Line::Index index, Line::Container&& tiles) :
    m_type(type),
    m_index(index),
    m_tiles(std::move(tiles))
{
}


Line& Line::operator=(const Line& other)
{
    const_cast<Line::Type&>(m_type) = other.m_type;
    const_cast<Line::Index&>(m_index) = other.m_index;
    m_tiles = other.m_tiles;
    return *this;
}


Line& Line::operator=(Line&& other) noexcept
{
    const_cast<Line::Type&>(m_type) = other.m_type;
    const_cast<Line::Index&>(m_index) = other.m_index;
    m_tiles = std::move(other.m_tiles);
    return *this;
}


Line operator+(const LineSpan& lhs, const LineSpan& rhs)
{
    assert(are_compatible(lhs, rhs));
    Line sum = line_from_line_span(lhs);
    LineSpanW sum_span(sum);
    sum_span += rhs;
    return sum;
}


Line operator-(const LineSpan& lhs, const LineSpan& rhs)
{
    assert(lhs.type() == rhs.type());
    assert(lhs.index() == rhs.index());
    assert(lhs.size() == rhs.size());
    Line delta = line_from_line_span(lhs);
    LineSpanW delta_span(delta);
    delta_span -= rhs;
    return delta;
}


bool is_line_uniform(const LineSpan& line, Tile color)
{
    return std::all_of(line.begin(), line.end(), [color](const Tile t) { return t == color; });
}


std::string str_line_type(Line::Type type)
{
    if (type == Line::ROW) { return "ROW"; }
    if (type == Line::COL) { return "COL"; }
    assert(0);
    return "ERR";
}

bool Line::is_completed() const
{
    return LineSpan(*this).is_completed();
}

bool operator==(const Line& lhs, const Line& rhs)
{
    return LineSpan(lhs) == LineSpan(rhs);
}


bool operator!=(const Line& lhs, const Line& rhs)
{
    return LineSpan(lhs) != LineSpan(rhs);
}


bool are_compatible(const Line& lhs, const Line& rhs)
{
    return are_compatible(LineSpan(lhs), LineSpan(rhs));
}


std::ostream& operator<<(std::ostream& out, const Line& line)
{
    return out << LineSpan(line);
}


std::string str_line_full(const Line& line)
{
    std::stringstream ss;
    ss << LineId(line) << " " << LineSpan(line);
    return ss.str();
}


std::ostream& operator<<(std::ostream& out, const LineId& line_id)
{
   return out << str_line_type(line_id.m_type) << " " << std::setw(3) << line_id.m_index;
}


InputGrid::Constraint get_constraint_from(const LineSpan& line_span)
{
    assert(line_span.is_completed());

    InputGrid::Constraint segments;
    unsigned int count = 0u;
    for (const auto& tile : line_span)
    {
        if (tile == Tile::FILLED) { count++; }
        if (tile == Tile::EMPTY && count > 0u) { segments.push_back(count); count = 0u; }
    }
    if (count > 0u) { segments.push_back(count); }          // Last but not least

    return segments;
}

InputGrid::Constraint get_constraint_from(const Line& line)
{
    return get_constraint_from(LineSpan(line));
}

Line line_from_line_span(const LineSpan& line_span)
{
    Line line(line_span.type(), line_span.index(), line_span.size());
    copy_line_span(line, line_span);
    return line;
}

const InputGrid::Constraints& get_constraints(const InputGrid& input_grid, Line::Type type)
{
    assert(type == Line::ROW || type == Line::COL);
    return type == Line::ROW ? input_grid.rows() : input_grid.cols();
}

const InputGrid::Constraint get_constraint(const InputGrid& input_grid, LineId line_id)
{
    return get_constraints(input_grid, line_id.m_type).at(line_id.m_index);
}

} // namespace picross
