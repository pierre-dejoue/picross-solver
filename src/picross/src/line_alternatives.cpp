#include "line_alternatives.h"

#include "binomial.h"
#include "line.h"
#include "line_constraint.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>

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

    template <bool Reversed>
    struct ConstraintIterator;

    template <>
    struct ConstraintIterator<false>
    {
        using Type = LineConstraint::Segments::const_iterator;
    };

    template <>
    struct ConstraintIterator<true>
    {
        using Type = LineConstraint::Segments::const_reverse_iterator;
    };

    template <bool Reversed>
    struct BidirectionalRange
    {
        using ConstraintIT = typename ConstraintIterator<Reversed>::Type;

        BidirectionalRange(const LineConstraint::Segments& segs, unsigned int line_length);

        ConstraintIT m_constraint_begin;
        ConstraintIT m_constraint_end;
        unsigned int m_line_begin;
        unsigned int m_line_end;
        unsigned int m_completed_segments;       // A completed segment is zero-terminated
        unsigned int m_nb_zeros;                 // Not counting the segments terminating zero
    };

    template <>
    BidirectionalRange<false>::BidirectionalRange(const LineConstraint::Segments& segs, unsigned int line_length)
        : m_constraint_begin(segs.cbegin())
        , m_constraint_end(segs.cend())
        , m_line_begin(0u)
        , m_line_end(line_length)
        , m_completed_segments(0u)
        , m_nb_zeros(0u)
    {
    }

    template <>
    BidirectionalRange<true>::BidirectionalRange(const LineConstraint::Segments& segs, unsigned int line_length)
        : m_constraint_begin(segs.crbegin())
        , m_constraint_end(segs.crend())
        , m_line_begin(0u)
        , m_line_end(line_length)
        , m_completed_segments(0u)
        , m_nb_zeros(0u)
    {
    }
}

struct LineAlternatives::Impl
{
    using Segments = LineConstraint::Segments;

    Impl(const LineConstraint& constraint, const Line& known_tiles, BinomialCoefficientsCache& binomial);

    template <bool Reversed>
    void reset();

    void reduce();

    template <bool Reversed>
    bool check_compatibility_bw(std::size_t start_idx, std::size_t end_idx) const;

    template <bool Reversed>
    BidirectionalRange<Reversed>& bidirectional_range();

    template <bool Reversed>
    bool update_range();

    bool update();

    template <bool Reversed>
    unsigned int reduce_alternatives(
        unsigned int remaining_zeros,
        typename ConstraintIterator<Reversed>::Type constraint_it,
        typename ConstraintIterator<Reversed>::Type constraint_partial_end,
        typename ConstraintIterator<Reversed>::Type constraint_end,
        std::size_t line_idx);

    Reduction reduce_all_alternatives();
    Reduction reduce_alternatives(unsigned int nb_constraints);

    const Segments&             m_segments;
    const Line&                 m_known_tiles;
    BinomialCoefficientsCache&  m_binomial;
    const unsigned int          m_line_length;
    const unsigned int          m_extra_zeros;
    unsigned int                m_remaining_zeros;
    BidirectionalRange<false>   m_bidirectional_range;
    BidirectionalRange<true>    m_bidirectional_range_reverse;
    Line                        m_alternative;
    std::unique_ptr<Line>       m_reduced_line;
};

LineAlternatives::Impl::Impl(const LineConstraint& constraints,  const Line& known_tiles, BinomialCoefficientsCache& binomial)
    : m_segments(constraints.segments())
    , m_known_tiles(known_tiles)
    , m_binomial(binomial)
    , m_line_length(static_cast<unsigned int>(known_tiles.size()))
    , m_extra_zeros(m_line_length - constraints.min_line_size())
    , m_remaining_zeros(m_extra_zeros)
    , m_bidirectional_range(constraints.segments(), m_line_length)
    , m_bidirectional_range_reverse(constraints.segments(), m_line_length)
    , m_alternative(known_tiles)
    , m_reduced_line()
{
    assert(m_alternative.index() == m_known_tiles.index());
    assert(m_alternative.type() == m_known_tiles.type());
    assert(m_alternative.size() == m_line_length);
    assert(m_line_length >= constraints.min_line_size());
}

template <bool Reversed>
void LineAlternatives::Impl::reset()
{
    const auto& range = bidirectional_range<Reversed>();
    m_reduced_line.reset();
    m_alternative = m_known_tiles;
    auto& tiles = m_alternative.tiles();
    for (auto line_idx = range.m_line_begin; line_idx < range.m_line_end; line_idx++)
        tiles[IndexTranslation<Reversed>()(m_line_length, line_idx)] = Tile::UNKNOWN;
}

void LineAlternatives::Impl::reduce()
{
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
bool LineAlternatives::Impl::check_compatibility_bw(std::size_t start_idx, std::size_t end_idx) const
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

template <>
BidirectionalRange<false>& LineAlternatives::Impl::bidirectional_range<false>() { return m_bidirectional_range; }

template <>
BidirectionalRange<true>& LineAlternatives::Impl::bidirectional_range<true>() { return m_bidirectional_range_reverse; }

template <bool Reversed>
bool LineAlternatives::Impl::update_range()
{
    // A completed segment is zero terminated in the exploration direction
    auto& range = bidirectional_range<Reversed>();
    const auto start_line_idx = range.m_line_begin;
    const auto end_line_idx = m_line_length;
    bool expect_termination_zero = false;
    unsigned int count_filled = 0;
    for (auto line_idx = start_line_idx; line_idx < end_line_idx; line_idx++)
    {
        const auto tile = m_known_tiles.tiles()[IndexTranslation<Reversed>()(m_line_length, line_idx)];
        if (tile == Tile::UNKNOWN)
            break;

        if (tile == Tile::EMPTY)
        {
            range.m_line_begin = line_idx + 1;
            if (expect_termination_zero)
            {
                expect_termination_zero = false;
                if (count_filled != *range.m_constraint_begin)
                    return false;
                count_filled = 0;
                range.m_completed_segments++;
                range.m_constraint_begin++;
            }
            else
            {
                range.m_nb_zeros++;
            }
        }
        else
        {
            assert(tile == Tile::FILLED);
            if (range.m_completed_segments == m_segments.size() - 1)
                break;
            expect_termination_zero = true;
            count_filled++;
        }
    }
    return true;
}

bool LineAlternatives::Impl::update()
{
    const bool valid_l = update_range<false>();
    const bool valid_r = update_range<true>();

    if (!valid_l || !valid_r)
        return false;

    return true;
}

template <bool Reversed>
unsigned int LineAlternatives::Impl::reduce_alternatives(
    unsigned int remaining_zeros,
    typename ConstraintIterator<Reversed>::Type constraint_it,
    typename ConstraintIterator<Reversed>::Type constraint_partial_end,
    typename ConstraintIterator<Reversed>::Type constraint_end,
    size_t line_idx)
{
    unsigned int nb_alternatives = 0u;

    // If the last segment of ones was reached, pad end of line with zeros
    if (constraint_it == constraint_end)
    {
        Line::Container& next_tiles = m_alternative.tiles();
        auto next_line_idx = line_idx;
        for (unsigned int c = 0u; c < remaining_zeros; c++) { next_tiles[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::EMPTY; }
        remaining_zeros = 0;
    }

    // If the last segment of this reduction was reached, check compatibility then reduce
    if (constraint_it == constraint_partial_end || constraint_it == constraint_end)
    {
        if (check_compatibility_bw<Reversed>(line_idx, m_alternative.size()))
        {
            reduce();
            const unsigned int nb_buckets = static_cast<unsigned int>(std::distance(constraint_partial_end, constraint_end) + 1);

            // TODO Check for overflow
            nb_alternatives += m_binomial.nb_alternatives_for_fixed_nb_of_partitions(remaining_zeros, nb_buckets);
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

        if (check_compatibility_bw<Reversed>(line_idx, next_line_idx))
        {
            nb_alternatives += reduce_alternatives<Reversed>(remaining_zeros, constraint_it + 1, constraint_partial_end, constraint_end, next_line_idx);
        }

        for (unsigned int pre_zeros = 1u; pre_zeros <= remaining_zeros; pre_zeros++)
        {
            next_tiles[IndexTranslation<Reversed>()(m_line_length, line_idx + pre_zeros - 1)] = Tile::EMPTY;
            next_line_idx = line_idx + pre_zeros + nb_ones - 1;
            next_tiles[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::FILLED;
            if (!is_last_constraint) { next_tiles[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::EMPTY; }

            if (check_compatibility_bw<Reversed>(line_idx, next_line_idx))
            {
                nb_alternatives += reduce_alternatives<Reversed>(remaining_zeros - pre_zeros, constraint_it + 1, constraint_partial_end, constraint_end, next_line_idx);
            }
        }
    }

    return nb_alternatives;
}

LineAlternatives::Reduction LineAlternatives::Impl::reduce_all_alternatives()
{
    reset<false>();
    const auto& range = m_bidirectional_range;
    const auto remaining_zeros = m_extra_zeros - range.m_nb_zeros;
    const auto nb_alternatives = reduce_alternatives<false>(remaining_zeros, range.m_constraint_begin, range.m_constraint_end, range.m_constraint_end, range.m_line_begin);
    const Line reduced_line = m_reduced_line ? *m_reduced_line : m_known_tiles;
    return Reduction { reduced_line, nb_alternatives, true };
}

LineAlternatives::Reduction LineAlternatives::Impl::reduce_alternatives(unsigned int nb_constraints)
{
    if (m_bidirectional_range.m_completed_segments + m_bidirectional_range_reverse.m_completed_segments + 2 * nb_constraints >= m_segments.size())
    {
        return reduce_all_alternatives();
    }

    const auto& range_l = m_bidirectional_range;
    const auto& range_r = m_bidirectional_range_reverse;
    typename ConstraintIterator<false>::Type constraint_l_end = range_l.m_constraint_begin;
    typename ConstraintIterator<true>::Type constraint_r_end = range_r.m_constraint_begin;
    while (nb_constraints-- > 0 ) {
        assert(constraint_l_end != range_l.m_constraint_end);
        constraint_l_end++;
        assert(constraint_r_end != range_r.m_constraint_end);
        constraint_r_end++;
    }

    // Reduce nb_constraints from left to right
    reset<false>();
    const auto remaining_zeros_l = m_extra_zeros - range_l.m_nb_zeros;
    const auto nb_alternatives_l = reduce_alternatives<false>(remaining_zeros_l, range_l.m_constraint_begin, constraint_l_end, range_l.m_constraint_end, range_l.m_line_begin);
    const Line reduced_line_l = m_reduced_line ? *m_reduced_line : m_known_tiles;

    // Reduce nb_constraints from right to left
    reset<true>();
    const auto remaining_zeros_r = m_extra_zeros - range_r.m_nb_zeros;
    const auto nb_alternatives_r = reduce_alternatives<true>(remaining_zeros_r, range_r.m_constraint_begin, constraint_r_end, range_r.m_constraint_end, range_r.m_line_begin);
    const Line reduced_line_r = m_reduced_line ? *m_reduced_line : m_known_tiles;

    return Reduction {
        reduced_line_l + reduced_line_r,
        std::min(nb_alternatives_l, nb_alternatives_r),
        false };
}

LineAlternatives::LineAlternatives(const LineConstraint& constraint, const Line& known_tiles, BinomialCoefficientsCache& binomial)
    : p_impl(std::make_unique<Impl>(constraint, known_tiles, binomial))
{
}

LineAlternatives::~LineAlternatives() = default;
LineAlternatives::LineAlternatives(LineAlternatives&&) noexcept = default;
LineAlternatives& LineAlternatives::operator=(LineAlternatives&&) noexcept = default;

LineAlternatives::Reduction LineAlternatives::full_reduction()
{
    const bool valid = p_impl->update();
    if (valid)
        return p_impl->reduce_all_alternatives();
    else
        return Reduction { p_impl->m_known_tiles, 0, true };
}

LineAlternatives::Reduction LineAlternatives::partial_reduction(unsigned int nb_constraints)
{
    assert(nb_constraints > 0);
    const bool valid = p_impl->update();
    if (valid)
        return p_impl->reduce_alternatives(nb_constraints);
    else
        return Reduction { p_impl->m_known_tiles, 0, true };
}

} // namespace picross
