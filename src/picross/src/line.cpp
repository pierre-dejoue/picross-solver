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
        else if (t1 == Tile::UNKNOWN) { return t2; }
        else { throw PicrossLineAdditionError(); }
    }

    inline Type compatible(Type t1, Type t2)
    {
        return t1 == Tile::UNKNOWN || t2 == Tile::UNKNOWN || t1 == t2;
    }

    inline Type delta(Type t1, Type t2)
    {
        if (t1 == t2) { return Tile::UNKNOWN; }
        else if (t1 == Tile::UNKNOWN) { return t2; }
        else { throw PicrossLineDeltaError(); }
    }

    inline Type reduce(Type t1, Type t2)
    {
        if (t1 == t2) { return t1; }
        else { return Tile::UNKNOWN; }
    }
} // namespace Tile


Line::Line(Line::Type type, size_t index, size_t size, Tile::Type init_tile) :
    type(type),
    index(index),
    tiles(size, init_tile)
{
}


Line::Line(const Line& other, Tile::Type init_tile) :
    Line(other.type, other.index, other.size(), init_tile)
{
}


Line::Line(Line::Type type, size_t index, const Line::Container& tiles) :
    type(type),
    index(index),
    tiles(tiles)
{
}


Line::Line(Line::Type type, size_t index, Line::Container&& tiles) :
    type(type),
    index(index),
    tiles(std::move(tiles))
{
}


Line::Type Line::get_type() const
{
    return type;
}


size_t Line::get_index() const
{
    return index;
}


const Line::Container& Line::get_tiles() const
{
    return tiles;
}


Line::Container& Line::get_tiles()
{
    return tiles;
}


size_t Line::size() const
{
    return tiles.size();
}


Tile::Type Line::at(size_t idx) const
{
    return tiles.at(idx);
}


/* Line::compatible() tests if two lines are compatible with each other
 */
bool Line::compatible(const Line& other) const
{
    if (other.type != type) { throw std::invalid_argument("compatible: Line type mismatch"); }
    if (other.index != index) { throw std::invalid_argument("compatible: Line index mismatch"); }
    if (other.tiles.size() != tiles.size()) { throw std::invalid_argument("compatible: Line size mismatch"); }
    for (size_t idx = 0u; idx < tiles.size(); ++idx)
    {
        if (!Tile::compatible(other.tiles.at(idx), tiles[idx]))
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
    if (other.type != type) { throw std::invalid_argument("add: Line type mismatch"); }
    if (other.index != index) { throw std::invalid_argument("add: Line index mismatch"); }
    if (other.tiles.size() != tiles.size()) { throw std::invalid_argument("add: Line size mismatch"); }
    const bool valid = compatible(other);
    if (valid)
    {
        std::transform(tiles.cbegin(), tiles.cend(), other.tiles.cbegin(), tiles.begin(), Tile::add);
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
    if (other.type != type) { throw std::invalid_argument("reduce: Line type mismatch"); }
    if (other.index != index) { throw std::invalid_argument("reduce: Line index mismatch"); }
    if (other.tiles.size() != tiles.size()) { throw std::invalid_argument("reduce: Line size mismatch"); }
    std::transform(tiles.begin(), tiles.end(), other.tiles.begin(), tiles.begin(), Tile::reduce);
}


/* line_delta computes the delta between line2 and line1, such that line2 = line1 + delta
 *
 * Example:
 *  (this) line2:    ....####..??
 *         line1:    ..????##..??
 *         delta:    ??..##??????
 */
Line line_delta(const Line& line1, const Line& line2)
{
    if (line1.get_type() != line2.get_type()) { throw std::invalid_argument("line_delta: Line type mismatch"); }
    if (line1.get_index() != line2.get_index()) { throw std::invalid_argument("line_delta: Line index mismatch"); }
    if (line1.size() != line2.size()) { throw std::invalid_argument("line_delta: Line size mismatch"); }
    std::vector<Tile::Type> delta_tiles;
    delta_tiles.resize(line1.size(), Tile::UNKNOWN);
    std::transform(line1.get_tiles().cbegin(), line1.get_tiles().cend(), line2.get_tiles().cbegin(), delta_tiles.begin(), Tile::delta);
    return Line(line1.get_type(), line1.get_index(), std::move(delta_tiles));
}


bool is_all_one_color(const Line& line, Tile::Type color)
{
    return std::all_of(line.get_tiles().cbegin(), line.get_tiles().cend(), [color](const Tile::Type t) { return t == color; });
}


bool is_fully_defined(const Line& line)
{
    return std::none_of(line.get_tiles().cbegin(), line.get_tiles().cend(), [](const Tile::Type t) { return t == Tile::UNKNOWN; });
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
    assert(0);
    return "UNKNOWN";
}


bool operator==(const Line& lhs, const Line& rhs)
{
    return lhs.get_type() == rhs.get_type()
        && lhs.get_index() == rhs.get_index()
        && lhs.get_tiles() == rhs.get_tiles();
}


bool operator!=(const Line& lhs, const Line& rhs)
{
    return !(lhs == rhs);
}


std::ostream& operator<<(std::ostream& ostream, const Line& line)
{
    std::ios prev_iostate(nullptr);
    prev_iostate.copyfmt(ostream);
    ostream << str_line_type(line.get_type()) << " " << std::setw(3) << line.get_index() << " " << str_line(line);
    ostream.copyfmt(prev_iostate);
    return ostream;
}

} // namespace picross
