#include "line_alternatives.h"

#include "binomial.h"
#include "line.h"
#include "line_constraint.h"
#include "line_span.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
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
        int operator()(const std::size_t line_sz, const int index) const;
    };

    template <>
    int IndexTranslation<false>::operator()(const std::size_t, const int index) const
    {
        return index;
    }

    template <>
    int IndexTranslation<true>::operator()(const std::size_t line_sz, const int index) const
    {
        assert(index < static_cast<int>(line_sz));
        return static_cast<int>(line_sz) - index - 1;
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
        int m_line_begin;
        int m_line_end;
        unsigned int m_completed_segments;       // From range begin. A completed segment is zero-terminated.
    };

    template <>
    BidirectionalRange<false>::BidirectionalRange(const Segments& segs, unsigned int line_length)
        : m_constraint_begin(segs.cbegin())
        , m_constraint_end(segs.cend())
        , m_line_begin(0)
        , m_line_end(static_cast<int>(line_length))
        , m_completed_segments(0u)
    {
    }

    template <>
    BidirectionalRange<true>::BidirectionalRange(const Segments& segs, unsigned int line_length)
        : m_constraint_begin(segs.crbegin())
        , m_constraint_end(segs.crend())
        , m_line_begin(0)
        , m_line_end(static_cast<int>(line_length))
        , m_completed_segments(0u)
    {
    }

    struct ReducedLine
    {
        ReducedLine(LineSpanW& line_span)
            : m_line(line_span)
            , m_reset(true)
        {}

        LineSpanW m_line;
        bool      m_reset;
    };

    LineAlternatives::Reduction from_line(const LineSpan& line, LineAlternatives::NbAlt nb_alt, bool full)
    {
        LineAlternatives::Reduction result;
        result.reduced_line = line_from_line_span(line);
        result.nb_alternatives = nb_alt;
        result.is_fully_reduced = full;
        return result;
    };

    // A Line with extra tiles at index -1 and line_sz
    class LineExt
    {
    public:
        LineExt(const LineSpan& line_span, Tile init_tile)
            : m_tiles(line_span.size() + 2, init_tile)
            , m_line_span(line_span.type(), line_span.index(), line_span.size(), m_tiles.data() + 1u)
        {
            m_tiles.front() = Tile::EMPTY;
            m_tiles.back()  = Tile::EMPTY;
        }

        LineExt(const LineSpan& line_span)
            : LineExt(line_span, Tile::UNKNOWN)
        {
            copy_line_span(m_line_span, line_span);
        }
        const LineSpanW& line_span() const { return m_line_span; }
        LineSpanW& line_span() { return m_line_span; }
    private:
        Line::Container m_tiles;
        LineSpanW       m_line_span;
    };
} // namespace


std::ostream& operator<<(std::ostream& out, const SegmentRange& segment_range)
{
    out << "SegmentRange{ left=" << segment_range.m_leftmost_index << ", right=" << segment_range.m_rightmost_index << " }";
    return out;
}

std::ostream& operator<<(std::ostream& out, const LineHole& line_hole)
{
    out << "LineHole{ idx=" << line_hole.m_index << ", len=" << line_hole.m_length << " }";
    return out;
}

std::vector<LineHole> line_holes(const LineSpan& known_tiles, int line_begin, int line_end)
{
    if (line_end == -1)
    {
        line_end = static_cast<int>(known_tiles.size());
    }
    assert(line_begin >= 0);
    assert(line_begin <= line_end);
    assert(line_end <= static_cast<int>(known_tiles.size()));
    std::vector<LineHole> result;
    bool new_hole = true;
    for (int index = line_begin; index < line_end; index++)
    {
        const auto tile = known_tiles[index];
        if (tile == Tile::EMPTY)
        {
            new_hole = true;
        }
        else
        {
            if (new_hole)
            {
                result.emplace_back(LineHole{ index, 1u });
                new_hole = false;
            }
            else
            {
                assert(!result.empty());
                result.back().m_length++;
            }
        }
    }
    assert(std::all_of(result.cbegin(), result.cend(), [](const LineHole& hole) { return hole.m_length > 0u; }));

    return result;
}


namespace
{
    std::pair<bool, std::vector<SegmentRange>> local_find_segments_range(const LineSpan& known_tiles, const BidirectionalRange<false>& range)
    {
        const auto nb_segments = static_cast<std::size_t>(std::distance(range.m_constraint_begin, range.m_constraint_end));
        std::vector<SegmentRange> result(nb_segments, SegmentRange{0, 0});
        bool success = false;

        auto l_holes = line_holes(known_tiles, range.m_line_begin, range.m_line_end);
        auto r_holes = l_holes;

        // Identify the leftmost configuration of the segments
        {
            auto constraint_it = range.m_constraint_begin;
            const auto constraint_end = range.m_constraint_end;
            auto hole_it = l_holes.begin();
            const auto hole_end = l_holes.end();
            assert(hole_it == hole_end || hole_it->m_index >= range.m_line_begin);
            auto result_seg_it = result.begin();
            const auto result_seg_end = result.end();
            while (constraint_it != constraint_end && hole_it != hole_end)
            {
                assert(result_seg_it != result_seg_end);
                const auto seg_len = *constraint_it;
                if (seg_len <= hole_it->m_length)
                {
                    // Found a fit for a segment!
                    result_seg_it->m_leftmost_index = hole_it->m_index;
                    result_seg_it++;
                    constraint_it++;
                    if (seg_len + 1u < hole_it->m_length)
                    {
                        hole_it->m_length -= seg_len + 1u;
                        hole_it->m_index += static_cast<int>(seg_len + 1u);
                    }
                    else
                    {
                        hole_it++;
                    }
                }
                else
                {
                    hole_it++;
                }
            }
            success = (constraint_it == constraint_end);
        }

        // Identify the rightmost configuration of the segments
        if (success)
        {
            auto constraint_it = std::make_reverse_iterator(range.m_constraint_end);
            const auto constraint_end = std::make_reverse_iterator(range.m_constraint_begin);
            auto hole_it = std::make_reverse_iterator(r_holes.end());
            const auto hole_end = std::make_reverse_iterator(r_holes.begin());
            assert(hole_it == hole_end || (hole_it->m_index + static_cast<int>(hole_it->m_length)) <= range.m_line_end);
            auto result_seg_it = std::make_reverse_iterator(result.end());
            const auto result_seg_end = std::make_reverse_iterator(result.begin());
            while (constraint_it != constraint_end && hole_it != hole_end)
            {
                assert(result_seg_it != result_seg_end);
                const auto seg_len = *constraint_it;
                if (seg_len <= hole_it->m_length)
                {
                    // Found a fit for a segment!
                    result_seg_it->m_rightmost_index = hole_it->m_index + static_cast<int>(hole_it->m_length - seg_len);
                    result_seg_it++;
                    constraint_it++;
                    if (seg_len + 1 < hole_it->m_length)
                    {
                        hole_it->m_length -= seg_len + 1u;
                        // hole_it->m_index stays identical
                    }
                    else
                    {
                        hole_it++;
                    }
                }
                else
                {
                    hole_it++;
                }
            }
            // If a leftmost configuration of the segments was found, then a rightmost configuration exists
            assert(constraint_it == constraint_end);
        }

        assert(!success || std::all_of(result.cbegin(), result.cend(), [](const auto& r) { return r.m_leftmost_index <= r.m_rightmost_index; }));
        return std::make_pair(success, result);
    }
} // namespace


struct LineAlternatives::Impl
{
    Impl(const LineConstraint& constraint, const LineSpan& known_tiles, BinomialCoefficients::Cache& binomial);
    Impl(const Impl& other, const LineSpan& known_tiles);

    void reduce(ReducedLine& line_reduce, const LineSpanW& alternative);

    template <bool Reversed>
    bool check_compatibility_bw(const LineSpanW& alternative, int start_idx, int end_idx) const;

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
        int line_begin,
        int line_end);

    template <bool Reversed>
    NbAlt reduce_alternatives_recursive(
        ReducedLine& reduced_line,
        LineSpanW& alternative,
        unsigned int remaining_zeros,
        typename ConstraintIterator<Reversed>::Type constraint_it,
        typename ConstraintIterator<Reversed>::Type constraint_partial_end,
        typename ConstraintIterator<Reversed>::Type constraint_end,
        int line_begin,
        int line_end);


    Reduction reduce_all_alternatives();
    Reduction reduce_alternatives(unsigned int nb_constraints);

    const Segments&                 m_segments;
    const LineSpan                  m_known_tiles;
    BinomialCoefficients::Cache&    m_binomial;
    const unsigned int              m_line_length;
    unsigned int                    m_remaining_zeros;
    BidirectionalRange<false>       m_bidirectional_range;
    BidirectionalRange<true>        m_bidirectional_range_reverse;
};

LineAlternatives::Impl::Impl(const LineConstraint& constraints, const LineSpan& known_tiles, BinomialCoefficients::Cache& binomial)
    : m_segments(constraints.segments())
    , m_known_tiles(known_tiles)
    , m_binomial(binomial)
    , m_line_length(static_cast<unsigned int>(known_tiles.size()))
    , m_remaining_zeros(m_line_length - constraints.min_line_size())
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
    , m_remaining_zeros(other.m_remaining_zeros)
    , m_bidirectional_range(other.m_bidirectional_range)
    , m_bidirectional_range_reverse(other.m_bidirectional_range_reverse)
{
    assert(m_line_length == m_known_tiles.size());
}

void LineAlternatives::Impl::reduce(ReducedLine& reduced_line, const LineSpanW& alternative)
{
    assert(are_compatible(LineSpan(alternative), m_known_tiles));
    if (reduced_line.m_reset)
    {
        reduced_line.m_reset = false;
        copy_line_span(reduced_line.m_line, LineSpan(alternative));
    }
    else
    {
        for (int idx = 0; idx < static_cast<int>(m_line_length); idx++)
        {
            auto& reduced_tile = reduced_line.m_line[idx];
            if (reduced_tile != Tile::UNKNOWN && reduced_tile != alternative[idx])
                reduced_tile = Tile::UNKNOWN;
        }
    }
}

template <bool Reversed>
bool LineAlternatives::Impl::check_compatibility_bw(const LineSpanW& alternative, int start_idx, int end_idx) const
{
    using TileRaw = unsigned char;
    static_assert(static_cast<TileRaw>(Tile::UNKNOWN) == 0);
    static constexpr TileRaw INCOMPATIBLE_SUM = static_cast<TileRaw>(Tile::EMPTY) + static_cast<TileRaw>(Tile::FILLED);
    assert(start_idx <= end_idx);
    for (int idx = start_idx; idx < end_idx; ++idx)
    {
        const auto trans_idx = IndexTranslation<Reversed>()(m_line_length, idx);
        if (static_cast<TileRaw>(m_known_tiles[trans_idx]) + static_cast<TileRaw>(alternative[trans_idx]) == INCOMPATIBLE_SUM)
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
    const auto end_line_idx = range.m_line_end;
    bool expect_termination_zero = false;
    unsigned int count_filled = 0;
    for (auto line_idx = start_line_idx; line_idx < end_line_idx; line_idx++)
    {
        const auto tile = m_known_tiles[IndexTranslation<Reversed>()(m_line_length, line_idx)];
        if (tile == Tile::UNKNOWN)
            break;

        if (tile == Tile::EMPTY)
        {
            if (expect_termination_zero)
            {
                expect_termination_zero = false;
                if (count_filled != *range.m_constraint_begin)
                    return false;
                count_filled = 0;
                range.m_completed_segments++;
                range.m_constraint_begin++;
            }
            range.m_line_begin = line_idx + 1;
        }
        else
        {
            assert(tile == Tile::FILLED);
            if (range.m_completed_segments == m_segments.size() - 1)
            {
                // Never fill the last segment (it may be non-zero terminated)
                break;
            }
            expect_termination_zero = true;
            count_filled++;
        }
    }
    // Validate that the remaining constraints can fit in the unknown part of the line
    assert(range.m_line_begin <= range.m_line_end);
    return true;
}

bool LineAlternatives::Impl::update()
{
    auto& range_l = bidirectional_range<false>();
    auto& range_r = bidirectional_range<true>();

    const bool valid_l = update_range<false>();
    if (!valid_l)
        return false;

    range_r.m_line_end = static_cast<int>(m_line_length) - range_l.m_line_begin;
    assert(range_r.m_line_begin <= range_r.m_line_end);
    assert(range_l.m_completed_segments == std::distance(m_segments.cbegin(), range_l.m_constraint_begin));
    range_r.m_constraint_end = m_segments.crend() - range_l.m_completed_segments;

    const bool valid_r = update_range<true>();
    if (!valid_r)
        return false;

    range_l.m_line_end = static_cast<int>(m_line_length) - range_r.m_line_begin;
    assert(range_r.m_completed_segments == std::distance(m_segments.crbegin(), range_r.m_constraint_begin));
    range_l.m_constraint_end = m_segments.cend() - range_r.m_completed_segments;

    assert(range_l.m_line_begin <= range_l.m_line_end);
    const auto min_line_size_l = compute_min_line_size(range_l.m_constraint_begin, range_l.m_constraint_end);
    const auto line_size_l = static_cast<unsigned int>(range_l.m_line_end - range_l.m_line_begin);
    assert(min_line_size_l == compute_min_line_size(range_r.m_constraint_begin, range_r.m_constraint_end));
    assert(line_size_l == static_cast<unsigned int>(range_r.m_line_end - range_r.m_line_begin));

    if (min_line_size_l > line_size_l)
        return false;

    m_remaining_zeros = line_size_l - min_line_size_l;

    // Assert that the beginnning of a range is equivalent to the end of the dual range
    assert(static_cast<unsigned int>(range_r.m_line_end + range_l.m_line_begin) == m_line_length);
    assert(static_cast<unsigned int>(range_l.m_line_end + range_r.m_line_begin) == m_line_length);

    return true;
}

template <bool Reversed>
std::pair<Line, LineAlternatives::NbAlt> LineAlternatives::Impl::reduce_alternatives(
    unsigned int remaining_zeros,
    typename ConstraintIterator<Reversed>::Type constraint_it,
    typename ConstraintIterator<Reversed>::Type constraint_partial_end,
    typename ConstraintIterator<Reversed>::Type constraint_end,
    int line_begin,
    int line_end)
{
    LineExt alternative_ext(m_known_tiles, Tile::UNKNOWN);
    LineExt reduced_line_ext(m_known_tiles, Tile::UNKNOWN);
    ReducedLine reduced_line(reduced_line_ext.line_span());
    LineSpanW& alternative = alternative_ext.line_span();
    auto nb_alternatives = reduce_alternatives_recursive<Reversed>(reduced_line, alternative, remaining_zeros, constraint_it, constraint_partial_end, constraint_end, line_begin, line_end);
    return std::make_pair<Line, NbAlt>(line_from_line_span(LineSpan(reduced_line.m_line)), std::move(nb_alternatives));
}


template <bool Reversed>
LineAlternatives::NbAlt LineAlternatives::Impl::reduce_alternatives_recursive(
    ReducedLine& reduced_line,
    LineSpanW& alternative,
    unsigned int remaining_zeros,
    typename ConstraintIterator<Reversed>::Type constraint_it,
    typename ConstraintIterator<Reversed>::Type constraint_partial_end,
    typename ConstraintIterator<Reversed>::Type constraint_end,
    int line_begin,
    int line_end)
{
    static_assert(std::is_same_v<NbAlt, BinomialCoefficients::Rep>);
    NbAlt nb_alternatives = 0u;

    // If the last segment of ones was reached, pad end of line with zeros
    if (constraint_it == constraint_end)
    {
        auto next_line_idx = line_begin;
        for (unsigned int c = 0u; c < remaining_zeros; c++) { alternative[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::EMPTY; }
        remaining_zeros = 0;
        assert(next_line_idx == line_end);
    }

    // If the last segment of this reduction was reached, check compatibility then reduce
    if (constraint_it == constraint_partial_end || constraint_it == constraint_end)
    {
        if (check_compatibility_bw<Reversed>(alternative, line_begin, line_end))
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
        const int nb_ones = static_cast<int>(*constraint_it);
        const bool is_last_constraint = (constraint_it + 1 == constraint_end);

        int next_line_idx = line_begin;
        for (auto c = 0; c < nb_ones; c++) { alternative[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::FILLED; }

        assert(nb_alternatives == 0);
        for (unsigned int pre_zeros = 0; pre_zeros <= remaining_zeros; pre_zeros++)
        {
            next_line_idx = line_begin + static_cast<int>(pre_zeros) - 1;
            alternative[IndexTranslation<Reversed>()(m_line_length, next_line_idx)] = Tile::EMPTY;
            next_line_idx += nb_ones;
            alternative[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::FILLED;
            if (!is_last_constraint) { alternative[IndexTranslation<Reversed>()(m_line_length, next_line_idx++)] = Tile::EMPTY; }

            assert(next_line_idx <= line_end);
            if (check_compatibility_bw<Reversed>(alternative, line_begin, next_line_idx))
            {
                BinomialCoefficients::add(nb_alternatives, reduce_alternatives_recursive<Reversed>(reduced_line, alternative, remaining_zeros - pre_zeros, constraint_it + 1, constraint_partial_end, constraint_end, next_line_idx, line_end));
            }
        }
    }
    return nb_alternatives;
}

LineAlternatives::Reduction LineAlternatives::Impl::reduce_all_alternatives()
{
    const auto& range_l = m_bidirectional_range;
    const auto [reduced_line, nb_alternatives] = reduce_alternatives<false>(m_remaining_zeros, range_l.m_constraint_begin, range_l.m_constraint_end, range_l.m_constraint_end, range_l.m_line_begin, range_l.m_line_end);
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
    while (nb_constraints-- > 0) {
        assert(constraint_l_end != range_l.m_constraint_end);
        constraint_l_end++;
        assert(constraint_r_end != range_r.m_constraint_end);
        constraint_r_end++;
    }

    // Reduce nb_constraints from left to right
    const auto [reduced_line_l, nb_alternatives_l] = reduce_alternatives<false>(m_remaining_zeros, range_l.m_constraint_begin, constraint_l_end, range_l.m_constraint_end, range_l.m_line_begin, range_l.m_line_end);

    // Reduce nb_constraints from right to left
    const auto [reduced_line_r, nb_alternatives_r] = reduce_alternatives<true>(m_remaining_zeros, range_r.m_constraint_begin, constraint_r_end, range_r.m_constraint_end, range_r.m_line_begin, range_r.m_line_end);

    auto nb_alternatives = std::min(nb_alternatives_l, nb_alternatives_r);

    if (nb_alternatives >= (std::numeric_limits<NbAlt>::max() >> 2))
    {
        const auto nb_unk = nb_unknown_tiles();
        if (nb_unk < std::numeric_limits<NbAlt>::digits)
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
        return from_line(p_impl->m_known_tiles, 0, true);
}

LineAlternatives::Reduction LineAlternatives::partial_reduction(unsigned int nb_constraints)
{
    assert(nb_constraints > 0);
    const bool valid = p_impl->update();
    if (valid)
        return p_impl->reduce_alternatives(nb_constraints);
    else
        return from_line(p_impl->m_known_tiles, 0, true);
}

LineAlternatives::Reduction LineAlternatives::linear_reduction()
{
    bool valid = p_impl->update();
    const auto& line_range = p_impl->m_bidirectional_range;
    std::vector<SegmentRange> ranges;
    const LineSpan& known_tiles = p_impl->m_known_tiles;
    if (valid)
    {
        auto segments_range = local_find_segments_range(known_tiles, line_range);
        valid = segments_range.first;
        ranges = std::move(segments_range.second);
    }
    if (!valid)
        return from_line(p_impl->m_known_tiles, 0, false);

    Reduction result = from_line(known_tiles, 1, false);

    LineExt empty_tiles_mask_extended_line(known_tiles, Tile::UNKNOWN);
    LineSpanW empty_tiles_mask = empty_tiles_mask_extended_line.line_span();
    for (int idx = line_range.m_line_begin; idx < line_range.m_line_end; idx++)
        empty_tiles_mask[idx] = Tile::EMPTY;                                    // empty_tiles_mask:  '0??000000?????0'

    LineExt reduction_mask_extended_line(known_tiles, Tile::UNKNOWN);
    LineSpanW reduction_mask = reduction_mask_extended_line.line_span();        // reduction_mask:    '0?????????????0'

    LineExt known_tiles_extended_line(known_tiles);
    const LineSpan known_tiles_ext(known_tiles_extended_line.line_span());      // known_tiles_ext:   '0<known_tiles>0'

    const auto nb_segments = ranges.size();
    auto constraint_it = p_impl->m_bidirectional_range.m_constraint_begin;
    const auto constraint_end = p_impl->m_bidirectional_range.m_constraint_end;
    for (std::size_t k = 0; k < nb_segments; k++)
    {
        NbAlt nb_alt = 0u;
        int min_index = static_cast<int>(known_tiles.size() + 1u);
        int max_index = -1;
        assert(constraint_it != constraint_end);
        const int seg_length = static_cast<int>(*constraint_it);
        for (int seg_index = ranges[k].m_leftmost_index; seg_index <= ranges[k].m_rightmost_index; seg_index++)
        {
            if (known_tiles_ext[seg_index - 1] == Tile::FILLED || known_tiles_ext[seg_index + seg_length] == Tile::FILLED)
                continue;  // Not a match
            bool match = true;
            for (int idx = seg_index; idx < seg_index + seg_length; idx++)
                if (known_tiles_ext[idx] == Tile::EMPTY) { match = false; break; }
            if (!match)
                continue;   // Not a match
            // Match!
            nb_alt++;
            min_index = std::min(min_index, seg_index);
            max_index = std::max(max_index, seg_index);
            for (int idx = seg_index; idx < seg_index + seg_length; idx++)
                empty_tiles_mask[idx] = Tile::UNKNOWN;
        }
        if (nb_alt == 1)
        {
            assert(min_index == max_index);
            reduction_mask[min_index - 1] = Tile::EMPTY;
            reduction_mask[min_index + seg_length] = Tile::EMPTY;
        }
        if (nb_alt > 0)
        {
            for (int idx = max_index; idx < min_index + seg_length; idx++)
                reduction_mask[idx] = Tile::FILLED;
        }
        BinomialCoefficients::mult(result.nb_alternatives, nb_alt);
        if (result.nb_alternatives == 0)
            break;
        constraint_it++;
    }
    result.reduced_line = result.reduced_line + LineSpan(reduction_mask);
    if (are_compatible(known_tiles, LineSpan(empty_tiles_mask)))
        result.reduced_line = result.reduced_line + LineSpan(empty_tiles_mask);
    else
        result.nb_alternatives = 0;
    result.is_fully_reduced = (result.nb_alternatives == 1);
    return result;
}

std::pair<bool, std::vector<SegmentRange>> LineAlternatives::find_segments_range() const
{
    BidirectionalRange<false> range(p_impl->m_segments, p_impl->m_line_length);
    return local_find_segments_range(p_impl->m_known_tiles, range);
}

} // namespace picross
