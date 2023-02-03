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


namespace picross
{

namespace
{
namespace Tiles
{
    inline Tile add(Tile t1, Tile t2)
    {
        if (t1 == t2 || t2 == Tile::UNKNOWN) { return t1; }
        else { assert(t1 == Tile::UNKNOWN); return t2; }
    }

    inline Tile delta(Tile t1, Tile t2)
    {
        if (t1 == t2) { return Tile::UNKNOWN; }
        else { assert(t2 == Tile::UNKNOWN); return t1; }
    }

    inline Tile reduce(Tile t1, Tile t2)
    {
        if (t1 == t2) { return t1; }
        return Tile::UNKNOWN;
    }
} // namespace Tiles
} // namespace


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


/* The addition combines the information of two lines into a single one.
 * Return false if the lines are not compatible, in which case 'this' is not modified.
 *
 * Example:
 *         line1:    ....##??????
 *         line2:    ..????##..??
 * line1 + line2:    ....####..??
 */
Line operator+(const LineSpan& lhs, const LineSpan& rhs)
{
    assert(are_compatible(lhs, rhs));
    Line sum = line_from_line_span(lhs);
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), &sum[0], Tiles::add);
    return sum;
}


/* The substraction computes the delta between lhs and rhs, such that lhs = rhs + delta
 *
 * Example:
 *  (this) lhs:      ....####..??
 *         rhs:      ..????##..??
 *         delta:    ??..##??????
 */
Line operator-(const LineSpan& lhs, const LineSpan& rhs)
{
    assert(lhs.type() == rhs.type());
    assert(lhs.index() == rhs.index());
    assert(lhs.size() == rhs.size());
    Line::Container delta_tiles(lhs.size(), Tile::UNKNOWN);
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), delta_tiles.begin(), Tiles::delta);
    return Line(lhs.type(), lhs.index(), std::move(delta_tiles));
}


/* line_reduce() captures the information that is common to two lines.
 *
 * Example:
 *           lhs:    ??..######..
 *           rhs:    ??....######
 *        reduce:    ??..??####??
 */
void line_reduce(LineSpanW& lhs, const LineSpan& rhs)
{
    assert(lhs.type() == rhs.type());
    assert(lhs.index() == rhs.index());
    assert(lhs.size() == rhs.size());
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), Tiles::reduce);
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


std::ostream& operator<<(std::ostream& ostream, const Line& line)
{
    return ostream << LineSpan(line);
}


std::string str_line_full(const Line& line)
{
    std::stringstream ss;
    ss << str_line_type(line.type()) << " " << std::setw(3) << line.index() << " " << LineSpan(line);
    return ss.str();
}


std::string str_line_id(const LineId& line_id)
{
    std::stringstream ss;
    ss << str_line_type(line_id.m_type) << " " << std::setw(3) << line_id.m_index;
    return ss.str();
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

void copy_line_span(LineSpanW& target, const LineSpan& source)
{
    assert(target.type() == source.type());
    assert(target.index() == source.index());
    assert(target.size() == source.size());
    const int sz = static_cast<int>(source.size());
    for (int idx = 0; idx < sz; idx++)
        target[idx] = source[idx];
}

void copy_line_span(Line& target, const LineSpan& source)
{
    LineSpanW target_w(target);
    copy_line_span(target_w, source);
}

Line line_from_line_span(const LineSpan& line_span)
{
    Line line(line_span.type(), line_span.index(), line_span.size());
    copy_line_span(line, line_span);
    return line;
}

} // namespace picross
