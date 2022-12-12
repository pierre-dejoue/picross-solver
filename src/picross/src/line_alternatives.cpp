#include "line_alternatives.h"

#include "line.h"

#include <cassert>
#include <cstddef>

namespace picross
{
namespace
{
    // Specialized for black and white puzzles
    inline bool partial_compatibility_bw(const Line& lhs, const Line& rhs, std::size_t start_idx, std::size_t end_idx)
    {
        const Line::Container& lhs_vect = lhs.tiles();
        const Line::Container& rhs_vect = rhs.tiles();
        for (size_t idx = start_idx; idx < end_idx; ++idx)
        {
            if ((lhs_vect[idx] == Tile::EMPTY && rhs_vect[idx] == Tile::FILLED) ||
                (lhs_vect[idx] == Tile::FILLED && rhs_vect[idx] == Tile::EMPTY))
            {
                return false;
            }
        }
        return true;
    }
}

struct LineAlternatives::Impl
{
    Impl(const InputGrid::Constraint& segs_of_ones, const Line& known_tiles);

    unsigned int build_alternatives(unsigned int remaining_zeros, const std::size_t line_idx = 0u, const std::size_t constraint_idx = 0u);
    void reduce();

    const InputGrid::Constraint&    m_segs_of_ones;
    const Line&                     m_known_tiles;
    Line                            m_alternative;
    std::unique_ptr<Line>           m_reduced_line;
};

LineAlternatives::Impl::Impl(const InputGrid::Constraint& segs_of_ones, const Line& known_tiles)
    : m_segs_of_ones(segs_of_ones)
    , m_known_tiles(known_tiles)
    , m_alternative(known_tiles, Tile::UNKNOWN)
    , m_reduced_line()
{
}

unsigned int LineAlternatives::Impl::build_alternatives(unsigned int remaining_zeros, const size_t line_idx, const size_t constraint_idx)
{
    assert(m_alternative.type() == m_known_tiles.type());
    assert(m_alternative.size() == m_known_tiles.size());

    unsigned int nb_alternatives = 0u;

    // If the last segment of ones was reached, pad end of line with zero, check compatibility then reduce
    if (constraint_idx == m_segs_of_ones.size())
    {
        Line::Container& next_tiles = m_alternative.tiles();
        assert(next_tiles.size() - line_idx == remaining_zeros);
        auto next_line_idx = line_idx;
        for (unsigned int c = 0u; c < remaining_zeros; c++) { next_tiles[next_line_idx++] = Tile::EMPTY; }
        assert(next_line_idx == next_tiles.size());

        if (partial_compatibility_bw(m_alternative, m_known_tiles, line_idx, next_line_idx))
        {
            reduce();
            nb_alternatives++;
        }
    }
    // Else, fill in the next segment of ones, then call recursively
    else
    {
        const auto& nb_ones = m_segs_of_ones[constraint_idx];
        Line::Container& next_tiles = m_alternative.tiles();

        auto next_line_idx = line_idx;
        for (unsigned int c = 0u; c < nb_ones; c++) { next_tiles[next_line_idx++] = Tile::FILLED; }
        if (constraint_idx + 1 < m_segs_of_ones.size()) { next_tiles[next_line_idx++] = Tile::EMPTY; }

        if (partial_compatibility_bw(m_alternative, m_known_tiles, line_idx, next_line_idx))
        {
            nb_alternatives += build_alternatives(remaining_zeros, next_line_idx, constraint_idx + 1);
        }

        for (unsigned int pre_zeros = 1u; pre_zeros <= remaining_zeros; pre_zeros++)
        {
            next_tiles[line_idx + pre_zeros - 1] = Tile::EMPTY;
            next_line_idx = line_idx + pre_zeros + nb_ones - 1;
            next_tiles[next_line_idx++] = Tile::FILLED;
            if (constraint_idx + 1 < m_segs_of_ones.size()) { next_tiles[next_line_idx++] = Tile::EMPTY; }

            if (partial_compatibility_bw(m_alternative, m_known_tiles, line_idx, next_line_idx))
            {
                nb_alternatives += build_alternatives(remaining_zeros - pre_zeros, next_line_idx, constraint_idx + 1);
            }
        }
    }

    return nb_alternatives;
}

void LineAlternatives::Impl::reduce()
{
    assert(is_complete(m_alternative));
    assert(m_alternative.compatible(m_known_tiles));
    if (!m_reduced_line)
    {
        m_reduced_line = std::make_unique<Line>(m_alternative);
    }
    else
    {
        m_reduced_line->reduce(m_alternative);
    }
}


LineAlternatives::LineAlternatives(const InputGrid::Constraint& segs_of_ones, const Line& known_tiles)
    : p_impl(std::make_unique<Impl>(segs_of_ones, known_tiles))
{
    assert(p_impl);
}

LineAlternatives::~LineAlternatives() = default;

unsigned int LineAlternatives::build_alternatives(unsigned int remaining_zeros)
{
    return p_impl->build_alternatives(remaining_zeros);
}

const Line& LineAlternatives::get_reduced_line()
{
    return p_impl->m_reduced_line ? *p_impl->m_reduced_line  : p_impl->m_known_tiles;
}

}