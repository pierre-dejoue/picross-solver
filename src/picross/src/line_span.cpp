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
} // namespace Tiles
} // namespace

bool LineSpan::is_completed() const
{
    return std::none_of(begin(), end(), [](const Tile t) { return t == Tile::UNKNOWN; });
}

bool LineSpan::compatible(const LineSpan& other) const
{
    assert(other.m_type == m_type);
    assert(other.m_index == m_index);
    assert(other.m_size == m_size);
    for (size_t idx = 0u; idx < m_size; ++idx)
    {
        if (!Tiles::compatible(other.m_tiles[idx], m_tiles[idx]))
            return false;
    }
    return true;
}

std::ostream& operator<<(std::ostream& ostream, const LineSpan& line)
{
    for (unsigned int idx = 0u; idx < line.size(); idx++)
        ostream << Tiles::str(line[idx]);
    return ostream;
}

bool are_compatible(const LineSpan& lhs, const LineSpan& rhs)
{
    return lhs.compatible(rhs);
}

} // namespace picross
