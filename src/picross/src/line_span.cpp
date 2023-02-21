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

    inline bool compatible(Tile t1, Tile t2)
    {
        return t1 == Tile::UNKNOWN || t2 == Tile::UNKNOWN || t1 == t2;
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

LineSpan::LineSpan(const LineSpanW& line_span)
    : LineSpan(line_span.type(), line_span.index(), line_span.size(), line_span.begin())
{
}

bool LineSpan::is_completed() const
{
    return std::none_of(begin(), end(), [](const Tile t) { return t == Tile::UNKNOWN; });
}

bool LineSpan::compatible(const LineSpan& other) const
{
    if (other.m_type != m_type)
        return false;
    if (other.m_index != m_index)
        return false;
    if (other.m_size != m_size)
        return false;
    for (size_t idx = 0u; idx < m_size; ++idx)
    {
        if (!Tiles::compatible(other.m_tiles[idx], m_tiles[idx]))
            return false;
    }
    return true;
}

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
    return lhs.compatible(rhs);
}

bool operator==(const LineSpanW& lhs, const LineSpanW& rhs)
{
    return lhs.type() == rhs.type()
        && lhs.index() == rhs.index()
        && lhs.size() == rhs.size()
        && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

bool operator!=(const LineSpanW& lhs, const LineSpanW& rhs)
{
    return !(lhs == rhs);
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
template <typename LineSpanT>
void line_add_impl(LineSpanW& lhs, const LineSpanT& rhs)
{
    assert(are_compatible(LineSpan(lhs), LineSpan(rhs)));
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), Tiles::add);
}
void line_add(LineSpanW& lhs, const LineSpan& rhs)  { line_add_impl(lhs, rhs); }
void line_add(LineSpanW& lhs, const LineSpanW& rhs) { line_add_impl(lhs, rhs); }

/* The substraction computes the delta between lhs and rhs, such that lhs = rhs + delta
 *
 * Example:
 *  (this) lhs:      ....####..??
 *         rhs:      ..????##..??
 *         delta:    ??..##??????
 */
template <typename LineSpanT>
void line_delta_impl(LineSpanW& lhs, const LineSpanT& rhs)
{
    assert(lhs.type() == rhs.type());
    assert(lhs.index() == rhs.index());
    assert(lhs.size() == rhs.size());
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), Tiles::delta);
}
void line_delta(LineSpanW& lhs, const LineSpan& rhs)  { line_delta_impl(lhs, rhs); }
void line_delta(LineSpanW& lhs, const LineSpanW& rhs) { line_delta_impl(lhs, rhs); }

/* line_reduce() captures the information that is common to two lines.
 *
 * Example:
 *           lhs:    ??..######..
 *           rhs:    ??....######
 *        reduce:    ??..??####??
 */
template <typename LineSpanT>
void line_reduce_impl(LineSpanW& lhs, const LineSpanT& rhs)
{
    assert(lhs.type() == rhs.type());
    assert(lhs.index() == rhs.index());
    assert(lhs.size() == rhs.size());
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), Tiles::reduce);
}
void line_reduce(LineSpanW& lhs, const LineSpan& rhs)  { line_reduce_impl(lhs, rhs); }
void line_reduce(LineSpanW& lhs, const LineSpanW& rhs) { line_reduce_impl(lhs, rhs); }

} // namespace picross
