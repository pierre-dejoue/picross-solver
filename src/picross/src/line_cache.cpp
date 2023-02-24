/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#include "line_cache.h"

#include "grid.h"
#include "line.h"

#include <picross/picross.h>

#include <cassert>
#include <vector>


namespace picross
{

class CacheGrid final : public Grid
{
public:
    CacheGrid(std::size_t width, std::size_t height)
        : Grid(width, height)
    {}

    template <typename LineSpanT>
    LineSpanT get_line(LineId line_id)
    {
        return Grid::get_line<LineSpanT>(line_id.m_type, line_id.m_index);
    }
};


struct LineCache::Impl
{
    Impl(std::size_t width, std::size_t height)
        : m_height(height)
        , m_cache_0(width, height)
        , m_cache_1(width, height)
        , m_nb_alts(2u * (width + height), LineAlternatives::NbAlt{0})
    {}

    LineAlternatives::NbAlt& nb_alts(LineId line_id, Tile key)
    {
        assert(key != Tile::UNKNOWN);
        const std::size_t index = 2 * ((line_id.m_type == Line::ROW ? 0 : m_height) + line_id.m_index) + (key == Tile::EMPTY ? 0u : 1u);
        return m_nb_alts[index];
    }

    std::size_t m_height;
    CacheGrid m_cache_0;
    CacheGrid m_cache_1;
    std::vector<LineAlternatives::NbAlt> m_nb_alts;
};

LineCache::LineCache(std::size_t width, std::size_t height)
    : p_impl(std::make_unique<Impl>(width, height))
{}

LineCache::~LineCache() = default;

LineCache::Entry LineCache::read_line(LineId line_id, Tile key) const
{
    assert(key != Tile::UNKNOWN);
    CacheGrid& cache = (key == Tile::EMPTY ? p_impl->m_cache_0 : p_impl->m_cache_1);
    return Entry{ cache.get_line<LineSpan>(line_id), p_impl->nb_alts(line_id, key) };
}

void LineCache::store_line(LineId line_id, Tile key, const LineSpan& line, LineAlternatives::NbAlt nb_alt)
{
    assert(key != Tile::UNKNOWN);
    CacheGrid& cache = (key == Tile::EMPTY ? p_impl->m_cache_0 : p_impl->m_cache_1);
    LineSpanW cache_line = cache.get_line<LineSpanW>(line_id);
    copy_line_span(cache_line, line);
    p_impl->nb_alts(line_id, key) = nb_alt;
}

} // namespace picross