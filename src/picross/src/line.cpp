/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#include "line.h"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>


namespace picross
{


namespace Tile
{
    char str(Type t)
    {
        if (t == UNKNOWN) { return '?'; }
        if (t == ZERO) { return '.'; }
        if (t == ONE) { return '#'; }
        std::ostringstream oss;
        oss << "Invalid tile value: " << t;
        throw std::invalid_argument(oss.str());
    }

    inline Type add(Type t1, Type t2)
    {
        if (t1 == t2 || t2 == Tile::UNKNOWN) { return t1; }
        else { assert(t1 == Tile::UNKNOWN); return t2; }
    }

    inline Type compatible(Type t1, Type t2)
    {
        return t1 == Tile::UNKNOWN || t2 == Tile::UNKNOWN || t1 == t2;
    }

    inline Type delta(Type t1, Type t2)
    {
        if (t1 == t2) { return Tile::UNKNOWN; }
        else { assert(t1 == Tile::UNKNOWN); return t2; }
    }

    inline Type reduce(Type t1, Type t2)
    {
        if (t1 == t2) { return t1; }
        else { return Tile::UNKNOWN; }
    }
} // namespace Tile


Line::Line(Line::Type type, size_t index, size_t size, Tile::Type init_tile) :
    m_type(type),
    m_index(index),
    m_tiles(size, init_tile)
{
}


Line::Line(const Line& other, Tile::Type init_tile) :
    Line(other.m_type, other.m_index, other.size(), init_tile)
{
}


Line::Line(Line::Type type, size_t index, const Line::Container& tiles) :
    m_type(type),
    m_index(index),
    m_tiles(tiles)
{
}


Line::Line(Line::Type type, size_t index, Line::Container&& tiles) :
    m_type(type),
    m_index(index),
    m_tiles(std::move(tiles))
{
}


Line::Type Line::type() const
{
    return m_type;
}


size_t Line::index() const
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


Tile::Type Line::at(size_t idx) const
{
    return m_tiles.at(idx);
}


/* Line::compatible() tests if two lines are compatible with each other
 */
bool Line::compatible(const Line& other) const
{
    assert(other.type() == type());
    assert(other.index() == index());
    assert(other.tiles().size() == tiles().size());
    for (size_t idx = 0u; idx < m_tiles.size(); ++idx)
    {
        if (!Tile::compatible(other.m_tiles.at(idx), m_tiles[idx]))
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
    assert(other.type() == type());
    assert(other.index() == index());
    assert(other.tiles().size() == tiles().size());
    const bool valid = compatible(other);
    if (valid)
    {
        std::transform(std::cbegin(m_tiles), std::cend(m_tiles), std::cbegin(other.m_tiles), std::begin(m_tiles), Tile::add);
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
    assert(other.type() == type());
    assert(other.index() == index());
    assert(other.tiles().size() == tiles().size());
    std::transform(std::cbegin(m_tiles), std::cend(m_tiles), std::cbegin(other.m_tiles), std::begin(m_tiles), Tile::reduce);
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
    std::vector<Tile::Type> delta_tiles;
    delta_tiles.resize(lhs.size(), Tile::UNKNOWN);
    std::transform(std::cbegin(lhs.tiles()), std::cend(lhs.tiles()), std::cbegin(rhs.tiles()), std::begin(delta_tiles), Tile::delta);
    return Line(lhs.type(), lhs.index(), std::move(delta_tiles));
}


bool is_all_one_color(const Line& line, Tile::Type color)
{
    return std::all_of(std::cbegin(line.tiles()), std::cend(line.tiles()), [color](const Tile::Type t) { return t == color; });
}


bool is_complete(const Line& line)
{
    return std::none_of(std::cbegin(line.tiles()), std::cend(line.tiles()), [](const Tile::Type t) { return t == Tile::UNKNOWN; });
}


std::string str_line(const Line& line)
{
    std::string out_str;
    for (unsigned int idx = 0u; idx < line.size(); idx++) { out_str += Tile::str(line.at(idx)); }
    return out_str;
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

} // namespace picross
