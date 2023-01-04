#include "line_alternatives.h"

#include "binomial.h"
#include "line.h"
#include "line_constraint.h"
#include "line_span.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <type_traits>
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
        using Type = Segments::const_iterator;
    };

    template <>
    struct ConstraintIterator<true>
    {
        using Type = Segments::const_reverse_iterator;
    };

    template <bool Reversed>
    struct BidirectionalRange
    {
        using ConstraintIT = typename ConstraintIterator<Reversed>::Type;

        BidirectionalRange(const Segments& segs, unsigned int line_length);

        ConstraintIT m_constraint_begin;
        ConstraintIT m_constraint_end;
        unsigned int m_line_begin;
        unsigned int m_line_end;
        unsigned int m_completed_segments;       // A completed segment is zero-terminated
        unsigned int m_nb_zeros;                 // Not counting the segments terminating zero
    };

    template <>
    BidirectionalRange<false>::BidirectionalRange(const Segments& segs, unsigned int line_length)
        : m_constraint_begin(segs.cbegin())
        , m_constraint_end(segs.cend())
        , m_line_begin(0u)
        , m_line_end(line_length)
        , m_completed_segments(0u)
        , m_nb_zeros(0u)
    {
    }

    template <>
    BidirectionalRange<true>::BidirectionalRange(const Segments& segs, unsigned int line_length)
        : m_constraint_begin(segs.crbegin())
        , m_constraint_end(segs.crend())
        , m_line_begin(0u)
        , m_line_end(line_length)
        , m_completed_segments(0u)
        , m_nb_zeros(0u)
    {
    }

    struct ReducedLine
    {
        ReducedLine(const LineSpan& templ)
            : m_line(templ.type(), templ.index(), templ.size())
            , m_reset(true)
        {}

        Line    m_line;
        bool    m_reset;
    };
}

struct LineAlternatives::Impl
{
    Impl(const LineConstraint& constraint, const LineSpan& known_tiles, BinomialCoefficients::Cache& binomial);
    Impl(const Impl& other, const LineSpan& known_tiles);

    template <bool Reversed>
    Line init_alternative();

    void reduce(ReducedLine& line_reduce, const Line& alternative);

    template <bool Reversed>
    bool check_compatibility_bw(const Line& alternative, std::size_t start_idx, std::size_t end_idx) const;

    unsigned int nb_unknown_tiles() const;

    template <bool Reversed>
    BidirectionalRange<Reversed>& bidirectional_range();

    template <bool Reversed>
    bool update_range();

    bool update();

    template <bool Reversed>
    std::pair<Line, NbAlt> reduce_alternatives(
        unsigned int remaining_zeros,
        typename ConstraintIterator<Reversed>::Type constraint_it,
        typename ConstraintIterator<Reversed>::Type constraint_partial_end,
        typename ConstraintIterator<Reversed>::Type constraint_end,
        std::size_t line_idx);

    template <bool Reversed>
    NbAlt reduce_alternatives_recursive(
        ReducedLine& reduced_line,
        Line& alternative,
        unsigned int remaining_zeros,
        typename ConstraintIterator<Reversed>::Type constraint_it,
        typename ConstraintIterator<Reversed>::Type constraint_partial_end,
        typename ConstraintIterator<Reversed>::Type constraint_end,
        std::size_t line_idx);


    Reduction reduce_all_alternatives();
    Reduction reduce_alternatives(unsigned int nb_constraints);

    const Segments&                 m_segments;
    const LineSpan                  m_known_tiles;
    BinomialCoefficients::Cache&    m_binomial;
    const unsigned int              m_line_length;
    const unsigned int              m_extra_zeros;
    BidirectionalRange<false>       m_bidirectional_range;
    BidirectionalRange<true>        m_bidirectional_range_reverse;
};

LineAlternatives::Impl::Impl(const LineConstraint& constraints,  const LineSpan& known_tiles, BinomialCoefficients::Cache& binomial)
    : m_segments(constraints.segments())
    , m_known_tiles(known_tiles)
    , m_binomial(binomial)
    , m_line_length(static_cast<unsigned int>(known_tiles.size()))
    , m_extra_zeros(m_line_length - constraints.min_line_size())
    , m_bidirectional_range(constraints.segments(), m_line_length)
    , m_bidirectional_range_reverse(constraints.segments(), m_line_length)
{
    assert(m_line_length >= constraints.min_line_size());
}

LineAlternatives::Impl::Impl(const Impl& other, const LineSpan& known_tiles)
    : m_segments(other.m_segments)
    , m_known_tiles(known_tiles)
    , m_binomial(other.m_binomial)
    , m_line_length(other.m_line_length)
    , m_extra_zeros(other.m_extra_zeros)
    , m_bidirectional_range(other.m_bidirectional_range)
    , m_bidirectional_range_reverse(other.m_bidirectional_range_reverse)
{
    assert(m_line_length == m_known_tiles.size());
}

template <bool Reversed>
Line LineAlternatives::Impl::init_alternative()
{
    Line alternative = line_from_line_span(m_known_tiles);
    const auto& range = bidirectional_range<Reversed>();
    for (auto line_idx = range.m_line_begin; line_idx < range.m_line_end; line_idx++)
        alternative[IndexTranslation<Reversed>()(m_line_length, line_idx)] = Tile::UNKNOWN;
    return alternative;
}

void LineAlternatives::Impl::reduce(ReducedLine& reduced_line, const Line& alternative)
{
    assert(are_compatible(alternative, m_known_tiles));
    if (reduced_line.m_reset)
    {
        reduced_line.m_reset = false;
        copy_line_from_line_span(reduced_line.m_line, alternative);
    }
    else
    {
        for (std::size_t idx = 0; idx < m_line_length; idx++)
        {
            auto& reduced_tile = reduced_line.m_line[idx];
            if (reduced_tile != Tile::UNKNOWN && reduced_tile != alternative.tiles()[idx])
                reduced_tile = Tile::UNKNOWN;
        }
    }
}

template <bool Reversed>
bool LineAlternatives::Impl::check_compatibility_bw(const Line& alternative, std::size_t start_idx, std::size_t end_idx) const
{
    using TileRaw = unsigned char;
    static_assert(static_cast<TileRaw>(Tile::UNKNOWN) == 0);
    static constexpr TileRaw INCOMPATIBLE_SUM = static_cast<TileRaw>(Tile::EMPTY) + static_cast<TileRaw>(Tile::FILLED);
    assert(start_idx <= end_idx);
    const Line::Container& alt_tiles = alternative.tiles();
    for (std::size_t idx = start_idx; idx < end_idx; ++idx)
    {
        const auto trans_idx = IndexTranslation<Reversed>()(m_line_length, idx);
        if (static_cast<TileRaw>(m_known_tiles[trans_idx]) + static_cast<TileRaw>(alt_tiles[trans_idx]) == INCOMPATIBLE_SUM)
            return false;
    }
    return true;
}

unsigned int LineAlternatives::Impl::nb_unknown_tiles() const
{
    const auto& range = m_bidirectional_range;
    unsigned int count = 0;
    for (auto idx = range.m_line_begin; idx < range.m_line_end; idx++)
    {
        if (m_known_tiles[idx] == Tile::UNKNOWN)
            count++;
    }
    return count;
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
        const auto tile = m_known_tiles[IndexTranslation<Reversed>()(m_line_length, line_idx)];
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
    // Validate that the remaining constraints can fit in the unknown part of the line
    assert(range.m_line_begin <= range.m_line_end);
    const bool valid = compute_min_line_size(range.m_constraint_begin, range.m_constraint_end) <= (range.m_line_end - range.m_line_begin);
    assert(!(valid && range.m_nb_zeros > m_extra_zeros));
    return valid;
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
std::pair<Line, LineAlternatives::NbAlt> LineAlternatives::Impl::reduce_alternatives(
    unsigned int remaining_zeros,
    typename ConstraintIterator<Reversed>::Type constraint_it,
    typename ConstraintIterator<Reversed>::Type constraint_partial_end,
    typename ConstraintIterator<Reversed>::Type constraint_end,
    size_t line_idx)
{
    ReducedLine reduced_line(m_known_tiles);
    Line alternative = init_alternative<Reversed>();
    auto nb_alternatives = reduce_alternatives_recursive<Reversed>(reduced_line, alternative, remaining_zeros, constraint_it, constraint_partial_end, constraint_end, line_idx);
    return std::make_pair<Line, NbAlt>(std::move(reduced_line.m_line), std::move(nb_alternatives));
}


template <bool Reversed>
LineAlternatives::NbAlt LineAlternatives::Impl::reduce_alternatives_recursive(
    ReducedLine& reduced_line,
    Line& alternative,
    unsigned int remaining_zeros,
    typename ConstraintIterator<Reversed>::Type constraint_it,
    typename ConstraintIterator<Reversed>::Type constraint_partial_end,
    typename ConstraintIterator<Reversed>::Type constraint_end,
    size_t line_idx)
{
    static_assert(std::is_same_v<NbAlt, BinomialCoefficients::Rep>);
    NbAlt nb_alternatives = 0u;

    // If the last segment of ones was reached, pad end of line with zeros
    if (constraint_it == constraint_end)
    {
        auto next_line_idx = line_idx;
        for (unsigned int c = 0u; c < remaining_zeros; c++) { alternative[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::EMPTY; }
        remaining_zeros = 0;
    }

    // If the last segment of this reduction was reached, check compatibility then reduce
    if (constraint_it == constraint_partial_end || constraint_it == constraint_end)
    {
        if (check_compatibility_bw<Reversed>(alternative, line_idx, m_line_length))
        {
            reduce(reduced_line, alternative);
            const unsigned int nb_buckets = static_cast<unsigned int>(std::distance(constraint_partial_end, constraint_end) + 1);
            assert(nb_alternatives == 0);
            nb_alternatives = remaining_zeros > 0 ? m_binomial.partition_n_elts_into_k_buckets(remaining_zeros, nb_buckets) : 1u;
        }
    }
    // Else, fill in the next segment of ones, then call recursively
    else
    {
        const auto nb_ones = *constraint_it;
        const bool is_last_constraint = (constraint_it + 1 == constraint_end);

        auto next_line_idx = line_idx;
        for (unsigned int c = 0u; c < nb_ones; c++) { alternative[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::FILLED; }
        if (!is_last_constraint) { alternative[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::EMPTY; }

        if (check_compatibility_bw<Reversed>(alternative, line_idx, next_line_idx))
        {
            assert(nb_alternatives == 0);
            nb_alternatives = reduce_alternatives_recursive<Reversed>(reduced_line, alternative, remaining_zeros, constraint_it + 1, constraint_partial_end, constraint_end, next_line_idx);
        }

        for (unsigned int pre_zeros = 1u; pre_zeros <= remaining_zeros; pre_zeros++)
        {
            alternative[IndexTranslation<Reversed>()(m_line_length, line_idx + pre_zeros - 1)] = Tile::EMPTY;
            next_line_idx = line_idx + pre_zeros + nb_ones - 1;
            alternative[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::FILLED;
            if (!is_last_constraint) { alternative[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::EMPTY; }

            if (check_compatibility_bw<Reversed>(alternative, line_idx, next_line_idx))
            {
                BinomialCoefficients::add(nb_alternatives, reduce_alternatives_recursive<Reversed>(reduced_line, alternative, remaining_zeros - pre_zeros, constraint_it + 1, constraint_partial_end, constraint_end, next_line_idx));
            }
        }
    }
    return nb_alternatives;
}

LineAlternatives::Reduction LineAlternatives::Impl::reduce_all_alternatives()
{
    const auto& range = m_bidirectional_range;
    assert(range.m_nb_zeros <= m_extra_zeros);
    const auto remaining_zeros = m_extra_zeros - range.m_nb_zeros;
    const auto [reduced_line, nb_alternatives] = reduce_alternatives<false>(remaining_zeros, range.m_constraint_begin, range.m_constraint_end, range.m_constraint_end, range.m_line_begin);
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
    assert(range_l.m_nb_zeros <= m_extra_zeros);
    const auto remaining_zeros_l = m_extra_zeros - range_l.m_nb_zeros;
    const auto [reduced_line_l, nb_alternatives_l] = reduce_alternatives<false>(remaining_zeros_l, range_l.m_constraint_begin, constraint_l_end, range_l.m_constraint_end, range_l.m_line_begin);

    // Reduce nb_constraints from right to left
    assert(range_r.m_nb_zeros <= m_extra_zeros);
    const auto remaining_zeros_r = m_extra_zeros - range_r.m_nb_zeros;
    const auto [reduced_line_r, nb_alternatives_r] = reduce_alternatives<true>(remaining_zeros_r, range_r.m_constraint_begin, constraint_r_end, range_r.m_constraint_end, range_r.m_line_begin);

    auto nb_alternatives = std::min(nb_alternatives_l, nb_alternatives_r);

    if (nb_alternatives == BinomialCoefficients::overflowValue())
    {
        const auto nb_unk = nb_unknown_tiles();
        if (nb_unk < std::numeric_limits<unsigned int>::digits)
            nb_alternatives = NbAlt{1} << nb_unk;
    }

    return Reduction {
        reduced_line_l + reduced_line_r,
        nb_alternatives,
        false
    };
}

LineAlternatives::LineAlternatives(const LineConstraint& constraint, const LineSpan& known_tiles, BinomialCoefficients::Cache& binomial)
    : p_impl(std::make_unique<Impl>(constraint, known_tiles, binomial))
{
}

LineAlternatives::LineAlternatives(const LineAlternatives& other, const LineSpan& known_tiles)
    : p_impl(std::make_unique<Impl>(*other.p_impl, known_tiles))
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
        return Reduction { ReducedLine(p_impl->m_known_tiles).m_line, 0, true };
}

LineAlternatives::Reduction LineAlternatives::partial_reduction(unsigned int nb_constraints)
{
    assert(nb_constraints > 0);
    const bool valid = p_impl->update();
    if (valid)
        return p_impl->reduce_alternatives(nb_constraints);
    else
        return Reduction { ReducedLine(p_impl->m_known_tiles).m_line, 0, true };
}

} // namespace picross
