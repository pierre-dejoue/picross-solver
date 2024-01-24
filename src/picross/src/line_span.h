/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include <picross/picross.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <type_traits>

namespace picross {

template <typename TileT>
class LineSpanImpl
{
public:
    LineSpanImpl(Line::Type type, Line::Index index, std::size_t size, TileT* tiles)
        : m_type(type)
        , m_index(index)
        , m_size(size)
        , m_tiles(tiles)
    {}

    // Implicit conversion from a const Line& to a LineSpan
    template <typename TileTCopy = TileT, std::enable_if_t<std::is_const_v<TileTCopy>, bool> = true>
    LineSpanImpl(const Line& line)
        : LineSpanImpl(line.type(), line.index(), line.size(), line.tiles())
    {}


    // Explicit conversion from a Line to a LineSpanW
    template <typename TileTCopy = TileT, std::enable_if_t<!std::is_const_v<TileTCopy>, bool> = true>
    explicit LineSpanImpl(Line& line)
        : LineSpanImpl(line.type(), line.index(), line.size(), line.tiles())
    {}

    // Implicit conversion from non-const Tile to const Tile
    template <typename TileU>
    LineSpanImpl(const LineSpanImpl<TileU>& other)
        : m_type(other.type())
        , m_index(other.index())
        , m_size(other.size())
        , m_tiles(other.begin())
    {}

    Line::Type type() const { return m_type; }
    Line::Index index() const { return m_index; }
    std::size_t size() const { return m_size; }
    TileT* tiles() const { return m_tiles; }
    TileT& operator[](int idx) const { return m_tiles[idx]; }
    TileT* begin() const { return m_tiles; }
    TileT* end() const { return m_tiles + m_size; }

    bool is_completed() const
    {
        return std::none_of(begin(), end(), [](const Tile t) { return t == Tile::UNKNOWN; });
    }

    LineSpanImpl head(int idx) const
    {
        assert(idx >= 0);
        return LineSpanImpl(m_type, m_index, static_cast<std::size_t>(idx), m_tiles);
    }

    LineSpanImpl tail(int idx) const
    {
        assert(static_cast<std::size_t>(idx) <= m_size);
        return LineSpanImpl(m_type, m_index, m_size - static_cast<std::size_t>(idx), m_tiles + idx);
    }

private:
    const Line::Type    m_type;
    const Line::Index   m_index;
    const std::size_t   m_size;
    TileT* const        m_tiles;
};

using LineSpan  = LineSpanImpl<const Tile>;         // A read-only Line span
using LineSpanW = LineSpanImpl<Tile>;               // A writable Line span

bool operator==(const LineSpan& lhs, const LineSpan& rhs);
bool operator!=(const LineSpan& lhs, const LineSpan& rhs);
bool are_compatible(const LineSpan& lhs, const LineSpan& rhs);
std::ostream& operator<<(std::ostream& out, const LineSpan& line);

LineSpanW& operator+=(LineSpanW& lhs, const LineSpan& rhs);
LineSpanW& operator-=(LineSpanW& lhs, const LineSpan& rhs);
LineSpanW& line_reduce(LineSpanW& lhs, const LineSpan& rhs);

} // namespace picross
