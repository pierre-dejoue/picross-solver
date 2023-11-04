/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#include "line_span.h"

#include <algorithm>
#include <cassert>
#include <sstream>

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

bool operator==(const LineSpan& lhs, const LineSpan& rhs)
{
    return lhs.type() == rhs.type()
        && lhs.index() == rhs.index()
        && lhs.size() == rhs.size()
        && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

bool operator!=(const LineSpan& lhs, const LineSpan& rhs)
{
    return !(lhs == rhs);
}

bool are_compatible(const LineSpan& lhs, const LineSpan& rhs)
{
    if (lhs.type() != rhs.type())
        return false;
    if (lhs.index() != rhs.index())
        return false;
    if (lhs.size() != rhs.size())
        return false;
    for (size_t idx = 0u; idx < lhs.size(); ++idx)
    {
        const auto& l_tile = lhs.tiles()[idx];
        const auto& r_tile = rhs.tiles()[idx];
        if (l_tile != Tile::UNKNOWN && r_tile != Tile::UNKNOWN && l_tile != r_tile)
            return false;
    }
    return true;
}

std::ostream& operator<<(std::ostream& out, const LineSpan& line)
{
    for (int idx = 0u; idx < static_cast<int>(line.size()); idx++)
        out << Tiles::str(line[idx]);
    return out;
}

/* The addition combines the information of two lines into a single one.
 * Return false if the lines are not compatible, in which case 'this' is not modified.
 *
 * Example:
 *         line1:    ....##??????
 *         line2:    ..????##..??
 * line1 + line2:    ....####..??
 */
LineSpanW& operator+=(LineSpanW& lhs, const LineSpan& rhs)
{
    assert(are_compatible(LineSpan(lhs), LineSpan(rhs)));
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), Tiles::add);
    return lhs;
}

/* The substraction computes the delta between lhs and rhs, such that lhs = rhs + delta
 *
 * Example:
 *  (this) lhs:      ....####..??
 *         rhs:      ..????##..??
 *         delta:    ??..##??????
 */
LineSpanW& operator-=(LineSpanW& lhs, const LineSpan& rhs)
{
    assert(are_compatible(LineSpan(lhs), LineSpan(rhs)));
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), Tiles::delta);
    return lhs;
}

/* line_reduce() captures the information that is common to two lines.
 *
 * Example:
 *           lhs:    ??..######..
 *           rhs:    ??....######
 *        reduce:    ??..??####??
 */
LineSpanW& line_reduce(LineSpanW& lhs, const LineSpan& rhs)
{
    assert(lhs.type() == rhs.type());
    assert(lhs.index() == rhs.index());
    assert(lhs.size() == rhs.size());
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), Tiles::reduce);
    return lhs;
}

} // namespace picross
