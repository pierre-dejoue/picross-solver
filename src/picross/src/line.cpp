/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#include "line.h"

#include <picross/picross.h>

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
    char str(Tile t)
    {
        if (t == Tile::UNKNOWN) { return '?'; }
        if (t == Tile::EMPTY) { return '.'; }
        if (t == Tile::FILLED) { return '#'; }
        std::ostringstream oss;
        oss << "Invalid tile value: " << static_cast<int>(t);
        throw std::invalid_argument(oss.str());
    }

    inline Tile add(Tile t1, Tile t2)
    {
        if (t1 == t2 || t2 == Tile::UNKNOWN) { return t1; }
        else { assert(t1 == Tile::UNKNOWN); return t2; }
    }

    inline bool compatible(Tile t1, Tile t2)
    {
        return t1 == Tile::UNKNOWN || t2 == Tile::UNKNOWN || t1 == t2;
    }

    inline Tile delta(Tile t1, Tile t2)
    {
        if (t1 == t2) { return Tile::UNKNOWN; }
        else { assert(t1 == Tile::UNKNOWN); return t2; }
    }

    inline Tile reduce(Tile t1, Tile t2)
    {
        if (t1 == t2) { return t1; }
        else { return Tile::UNKNOWN; }
    }
} // namespace Tiles
} // namespace


Line::Line(Line::Type type, Line::Index index, size_t size, Tile init_tile) :
    m_type(type),
    m_index(index),
    m_tiles(size, init_tile)
{
}


Line::Line(const Line& other, Tile init_tile) :
    Line(other.m_type, other.m_index, other.size(), init_tile)
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


Line::Type Line::type() const
{
    return m_type;
}


Line::Index Line::index() const
{
    return m_index;
}


const Line::Container& Line::tiles() const
{
    return m_tiles;
}


Line::Container& Line::tiles()
{
    return m_tiles;
}


size_t Line::size() const
{
    return m_tiles.size();
}


Tile Line::at(Line::Index idx) const
{
    return m_tiles.at(idx);
}


/* Line::compatible() tests if two lines are compatible with each other
 */
bool Line::compatible(const Line& other) const
{
    assert(other.m_type == m_type);
    assert(other.m_index == m_index);
    assert(other.size() == size());
    for (size_t idx = 0u; idx < m_tiles.size(); ++idx)
    {
        if (!Tiles::compatible(other.m_tiles[idx], m_tiles[idx]))
        {
            return false;
        }
    }
    return true;
}

/* Line::add() combines the information of two lines into a single one.
 * Return false if the lines are not compatible, in which case 'this' is not modified.
 *
 * Example:
 *         line1:    ....##??????
 *         line2:    ..????##..??
 * line1 + line2:    ....####..??
 */
bool Line::add(const Line& other)
{
    assert(other.m_type == m_type);
    assert(other.m_index == m_index);
    assert(other.size() == size());
    const bool valid = compatible(other);
    if (valid)
    {
        std::transform(std::cbegin(m_tiles), std::cend(m_tiles), std::cbegin(other.m_tiles), std::begin(m_tiles), Tiles::add);
    }
    return valid;
}


/* Line::reduce() captures the information that is common to two lines.
 *
 * Example:
 *         line1:    ??..######..
 *         line2:    ??....######
 * line1 ^ line2:    ??..??####??
 */
void Line::reduce(const Line& other)
{
    assert(other.m_type == m_type);
    assert(other.m_index == m_index);
    assert(other.size() == size());
    std::transform(std::cbegin(m_tiles), std::cend(m_tiles), std::cbegin(other.m_tiles), std::begin(m_tiles), Tiles::reduce);
}


/* line_delta computes the delta between line2 and line1, such that line2 = line1 + delta
 *
 * Example:
 *  (this) line2:    ....####..??
 *         line1:    ..????##..??
 *         delta:    ??..##??????
 */
Line line_delta(const Line& lhs, const Line& rhs)
{
    assert(lhs.type() == rhs.type());
    assert(lhs.index() == rhs.index());
    assert(lhs.size() == rhs.size());
    std::vector<Tile> delta_tiles;
    delta_tiles.resize(lhs.size(), Tile::UNKNOWN);
    std::transform(std::cbegin(lhs.tiles()), std::cend(lhs.tiles()), std::cbegin(rhs.tiles()), std::begin(delta_tiles), Tiles::delta);
    return Line(lhs.type(), lhs.index(), std::move(delta_tiles));
}


bool is_all_one_color(const Line& line, Tile color)
{
    return std::all_of(std::cbegin(line.tiles()), std::cend(line.tiles()), [color](const Tile t) { return t == color; });
}


bool is_complete(const Line& line)
{
    return std::none_of(std::cbegin(line.tiles()), std::cend(line.tiles()), [](const Tile t) { return t == Tile::UNKNOWN; });
}


std::string str_line(const Line& line)
{
    std::stringstream ss;
    for (unsigned int idx = 0u; idx < line.size(); idx++)
    {
        ss << Tiles::str(line.at(idx));
    }
    return ss.str();
}


std::string str_line_type(Line::Type type)
{
    if (type == Line::ROW) { return "ROW"; }
    if (type == Line::COL) { return "COL"; }
    assert(false);
    return "UNKNOWN";
}


bool operator==(const Line& lhs, const Line& rhs)
{
    return lhs.type() == rhs.type()
        && lhs.index() == rhs.index()
        && lhs.tiles() == rhs.tiles();
}


bool operator!=(const Line& lhs, const Line& rhs)
{
    return !(lhs == rhs);
}


std::ostream& operator<<(std::ostream& ostream, const Line& line)
{
    std::ios prev_iostate(nullptr);
    prev_iostate.copyfmt(ostream);
    ostream << str_line_type(line.type()) << " " << std::setw(3) << line.index() << " " << str_line(line);
    ostream.copyfmt(prev_iostate);
    return ostream;
}

InputGrid::Constraint get_constraint_from(const Line& line)
{
    assert(is_complete(line));

    InputGrid::Constraint segments;
    unsigned int count = 0u;
    for (const auto& tile : line.tiles())
    {
        if (tile == Tile::FILLED) { count++; }
        if (tile == Tile::EMPTY && count > 0u) { segments.push_back(count); count = 0u; }
    }
    if (count > 0u) { segments.push_back(count); }          // Last but not least

    return segments;
}

Line operator+(const Line& lhs, const Line& rhs)
{
    Line sum = lhs;
    const bool valid = sum.add(rhs);
    assert(valid);
    return sum;
}

} // namespace picross
