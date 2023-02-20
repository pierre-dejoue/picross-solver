#pragma once

#include <picross/picross.h>

#include <cassert>
#include <cstddef>


namespace picross
{

class LineSpanW;
class LineSpan
{
public:
    LineSpan(Line::Type type, Line::Index index, std::size_t size, const Tile* tiles)
        : m_type(type)
        , m_index(index)
        , m_size(size)
        , m_tiles(tiles)
    {}

    // Implicit conversion from Line to LineSpan
    LineSpan(const Line& line)
        : LineSpan(line.type(), line.index(), line.size(), line.tiles())
    {}

    explicit LineSpan(const LineSpanW& line_span);

    Line::Type type() const { return m_type; }
    Line::Index index() const { return m_index; }
    std::size_t size() const { return m_size; }
    const Tile& operator[](int idx) const { return m_tiles[idx]; }
    const Tile* begin() const { return m_tiles; }
    const Tile* end() const { return m_tiles + m_size; }
    bool is_completed() const;
    bool compatible(const LineSpan& other) const;

    LineSpan head(int idx) const
    {
        assert(idx >= 0);
        return LineSpan(m_type, m_index, static_cast<std::size_t>(idx), m_tiles);
    }
    LineSpan tail(int idx) const
    {
        assert(static_cast<std::size_t>(idx) <= m_size);
        return LineSpan(m_type, m_index, m_size - static_cast<std::size_t>(idx), m_tiles + idx);
    }

private:
    const Line::Type    m_type;
    const Line::Index   m_index;
    const std::size_t   m_size;
    const Tile* const   m_tiles;
};

bool operator==(const LineSpan& lhs, const LineSpan& rhs);
bool operator!=(const LineSpan& lhs, const LineSpan& rhs);
bool are_compatible(const LineSpan& lhs, const LineSpan& rhs);
std::ostream& operator<<(std::ostream& ostream, const LineSpan& line);


class LineSpanW
{
public:
    LineSpanW(Line::Type type, Line::Index index, std::size_t size, Tile* tiles)
        : m_type(type)
        , m_index(index)
        , m_size(size)
        , m_tiles(tiles)
    {}

    // Explicit conversion from Line to LineSpan
    explicit LineSpanW(Line& line)
        : LineSpanW(line.type(), line.index(), line.size(), line.tiles())
    {}

    Line::Type type() const { return m_type; }
    Line::Index index() const { return m_index; }
    std::size_t size() const { return m_size; }
    const Tile& operator[](int idx) const { return m_tiles[idx]; }
    const Tile* begin() const { return m_tiles; }
    const Tile* end() const { return m_tiles + m_size; }

    Tile& operator[](int idx) { return m_tiles[idx]; }
    Tile* begin() { return m_tiles; }
    Tile* end() { return m_tiles + m_size; }

    template <typename LineSpanT>
    LineSpanW& operator+=(const LineSpanT& rhs);
    template <typename LineSpanT>
    LineSpanW& operator-=(const LineSpanT& rhs);

    LineSpanW head(int idx)
    {
        assert(idx >= 0);
        return LineSpanW(m_type, m_index, static_cast<std::size_t>(idx), m_tiles);
    }

    LineSpanW tail(int idx)
    {
        assert(static_cast<std::size_t>(idx) <= m_size);
        return LineSpanW(m_type, m_index, m_size - static_cast<std::size_t>(idx), m_tiles + idx);
    }

private:
    const Line::Type    m_type;
    const Line::Index   m_index;
    const std::size_t   m_size;
    Tile* const         m_tiles;
};

bool operator==(const LineSpanW& lhs, const LineSpanW& rhs);
bool operator!=(const LineSpanW& lhs, const LineSpanW& rhs);

void line_add(LineSpanW& lhs, const LineSpan& rhs);
void line_add(LineSpanW& lhs, const LineSpanW& rhs);
void line_delta(LineSpanW& lhs, const LineSpan& rhs);
void line_delta(LineSpanW& lhs, const LineSpanW& rhs);
void line_reduce(LineSpanW& lhs, const LineSpan& rhs);
void line_reduce(LineSpanW& lhs, const LineSpanW& rhs);

template <typename LineSpanT>
LineSpanW& LineSpanW::operator+=(const LineSpanT& rhs)
{
    line_add(*this, rhs);
    return *this;
}

template <typename LineSpanT>
LineSpanW& LineSpanW::operator-=(const LineSpanT& rhs)
{
    line_delta(*this, rhs);
    return *this;
}

} // namespace picross
