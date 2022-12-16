#include "line_alternatives.h"

#include "line.h"
#include "line_constraint.h"

#include <cassert>
#include <cstddef>

namespace picross
{

namespace
{
    template <bool Reversed>
    struct IndexTranslation
    {
        std::size_t operator()(const std::size_t line_sz, const std::size_t index) const;
    };

    template <>
    std::size_t IndexTranslation<false>::operator()(const std::size_t, const std::size_t index) const
    {
        return index;
    }

    template <>
    std::size_t IndexTranslation<true>::operator()(const std::size_t line_sz, const std::size_t index) const
    {
        assert(index < line_sz);
        return line_sz - index - 1;
    }
}

struct LineAlternatives::Impl
{
    Impl(const LineConstraint& constraint, const Line& known_tiles);
    virtual ~Impl() = 0;

    void reset();
    virtual unsigned int build_alternatives() = 0;
    void reduce();

    const unsigned int                  m_line_length;
    const std::vector<unsigned int>&    m_segs_of_ones;
    const Line&                         m_known_tiles;
    unsigned int                        m_remaining_zeros;
    Line                                m_alternative;
    std::unique_ptr<Line>               m_reduced_line;
};

template <bool Reversed>
struct LineAlternatives::BidirectionalImpl : public LineAlternatives::Impl
{
    BidirectionalImpl(const LineConstraint& constraint, const Line& known_tiles);

    unsigned int build_alternatives() override;

    bool check_compatibility_bw(std::size_t start_idx, std::size_t end_idx) const;

    template <typename ConstraintIT>
    unsigned int build_alternatives(unsigned int remaining_zeros, ConstraintIT constraint_it, ConstraintIT constraint_end, std::size_t line_idx = 0);
};


LineAlternatives::Impl::Impl(const LineConstraint& constraints,  const Line& known_tiles)
    : m_line_length(static_cast<unsigned int>(known_tiles.size()))
    , m_segs_of_ones(constraints.segments())
    , m_known_tiles(known_tiles)
    , m_remaining_zeros(0u)
    , m_alternative(known_tiles, Tile::UNKNOWN)
    , m_reduced_line()
{
    assert(m_alternative.index() == m_known_tiles.index());
    assert(m_alternative.type() == m_known_tiles.type());
    assert(m_alternative.size() == m_line_length);

    assert(m_line_length >= constraints.min_line_size());
    m_remaining_zeros = m_line_length - constraints.min_line_size();
}

LineAlternatives::Impl::~Impl() = default;

void LineAlternatives::Impl::reset()
{
    m_reduced_line.reset();
    for (auto& t : m_alternative.tiles()) { t = Tile::UNKNOWN; }
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


template <bool Reversed>
LineAlternatives::BidirectionalImpl<Reversed>::BidirectionalImpl(const LineConstraint& constraint, const Line& known_tiles)
    : Impl(constraint, known_tiles)
{
}

template <>
unsigned int LineAlternatives::BidirectionalImpl<false>::build_alternatives()
{
    return build_alternatives(m_remaining_zeros, m_segs_of_ones.cbegin(), m_segs_of_ones.cend());
}

template <>
unsigned int LineAlternatives::BidirectionalImpl<true>::build_alternatives()
{
    return build_alternatives(m_remaining_zeros, m_segs_of_ones.crbegin(), m_segs_of_ones.crend());
}

template <bool Reversed>
bool LineAlternatives::BidirectionalImpl<Reversed>::check_compatibility_bw(std::size_t start_idx, std::size_t end_idx) const
{
    assert(start_idx <= end_idx);
    const Line::Container& lhs_vect = m_known_tiles.tiles();
    const Line::Container& rhs_vect = m_alternative.tiles();
    for (size_t idx = start_idx; idx < end_idx; ++idx)
    {
        const auto trans_idx = IndexTranslation<Reversed>()(m_line_length, idx);
        if ((lhs_vect[trans_idx] == Tile::EMPTY && rhs_vect[trans_idx] == Tile::FILLED) ||
            (lhs_vect[trans_idx] == Tile::FILLED && rhs_vect[trans_idx] == Tile::EMPTY))
        {
            return false;
        }
    }
    return true;
}

template <bool Reversed>
template <typename ConstraintIT>
unsigned int LineAlternatives::BidirectionalImpl<Reversed>::build_alternatives(unsigned int remaining_zeros, ConstraintIT constraint_it, ConstraintIT constraint_end, size_t line_idx)
{
    unsigned int nb_alternatives = 0u;

    // If the last segment of ones was reached, pad end of line with zero, check compatibility then reduce
    if (constraint_it == constraint_end)
    {
        Line::Container& next_tiles = m_alternative.tiles();
        assert(next_tiles.size() - line_idx == remaining_zeros);
        auto next_line_idx = line_idx;
        for (unsigned int c = 0u; c < remaining_zeros; c++) { next_tiles[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::EMPTY; }
        assert(next_line_idx == next_tiles.size());

        if (check_compatibility_bw(line_idx, next_line_idx))
        {
            reduce();
            nb_alternatives++;
        }
    }
    // Else, fill in the next segment of ones, then call recursively
    else
    {
        const auto nb_ones = *constraint_it;
        const bool is_last_constraint = (constraint_it + 1 == constraint_end);
        Line::Container& next_tiles = m_alternative.tiles();

        auto next_line_idx = line_idx;
        for (unsigned int c = 0u; c < nb_ones; c++) { next_tiles[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::FILLED; }
        if (!is_last_constraint) { next_tiles[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::EMPTY; }

        if (check_compatibility_bw(line_idx, next_line_idx))
        {
            nb_alternatives += build_alternatives(remaining_zeros, constraint_it + 1, constraint_end, next_line_idx);
        }

        for (unsigned int pre_zeros = 1u; pre_zeros <= remaining_zeros; pre_zeros++)
        {
            next_tiles[IndexTranslation<Reversed>()(m_line_length, line_idx + pre_zeros - 1)] = Tile::EMPTY;
            next_line_idx = line_idx + pre_zeros + nb_ones - 1;
            next_tiles[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::FILLED;
            if (!is_last_constraint) { next_tiles[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::EMPTY; }

            if (check_compatibility_bw(line_idx, next_line_idx))
            {
                nb_alternatives += build_alternatives(remaining_zeros - pre_zeros, constraint_it + 1, constraint_end, next_line_idx);
            }
        }
    }

    return nb_alternatives;
}


LineAlternatives::LineAlternatives(const LineConstraint& constraint, const Line& known_tiles, bool reversed)
{
    if (reversed)
        p_impl = std::make_unique<BidirectionalImpl<true>>(constraint, known_tiles);
    else
        p_impl = std::make_unique<BidirectionalImpl<false>>(constraint, known_tiles);
    assert(p_impl);
}

LineAlternatives::~LineAlternatives() = default;
LineAlternatives::LineAlternatives(LineAlternatives&&) noexcept = default;
LineAlternatives& LineAlternatives::operator=(LineAlternatives&&) noexcept = default;

LineAlternatives::Reduction LineAlternatives::full_reduction()
{
    p_impl->reset();
    const unsigned int nb_alternatives = p_impl->build_alternatives();
    const Line reduced_line = p_impl->m_reduced_line ? *p_impl->m_reduced_line  : p_impl->m_known_tiles;
    return Reduction { reduced_line, nb_alternatives, true };
}

}