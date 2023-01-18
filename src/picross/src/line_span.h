#pragma once

#include <picross/picross.h>

#include <cstddef>


namespace picross
{

class LineSpan
{
public:
    LineSpan(Line::Type type, Line::Index index, const std::vector<Tile>& vect)
        : m_type(type)
        , m_index(index)
        , m_size(vect.size())
        , m_tiles(vect.data())
    {}

    LineSpan(Line::Type type, Line::Index index, std::size_t size, const Tile* tiles)
        : m_type(type)
        , m_index(index)
        , m_size(size)
        , m_tiles(tiles)
    {}

    // Implicit conversion from Line to LineSpan
    LineSpan(const Line& line)
        : LineSpan(line.type(), line.index(), line.tiles())
    {}

    Line::Type type() const { return m_type; }
    Line::Index index() const { return m_index; }
    std::size_t size() const { return m_size; }
    const Tile& operator[](std::size_t idx) const { return m_tiles[idx]; }
    const Tile* begin() const { return m_tiles; }
    const Tile* end() const { return m_tiles + m_size; }

    bool is_completed() const;
    bool compatible(const LineSpan& other) const;

private:
    const Line::Type    m_type;
    const Line::Index   m_index;
    const std::size_t   m_size;
    const Tile* const   m_tiles;
};

std::ostream& operator<<(std::ostream& ostream, const LineSpan& line);

bool are_compatible(const LineSpan& lhs, const LineSpan& rhs);

} // namespace picross