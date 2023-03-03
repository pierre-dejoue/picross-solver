#include "line_alternatives.h"

#include "binomial.h"
#include "line.h"
#include "line_constraint.h"
#include "line_span.h"

#include <stdutils/span.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <tuple>
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
    };

    template <>
    BidirectionalRange<false>::BidirectionalRange(const Segments& segs, unsigned int line_length)
        : m_constraint_begin(segs.cbegin())
        , m_constraint_end(segs.cend())
        , m_line_begin(0)
        , m_line_end(static_cast<int>(line_length))
    {
    }

    template <>
    BidirectionalRange<true>::BidirectionalRange(const Segments& segs, unsigned int line_length)
        : m_constraint_begin(segs.crbegin())
        , m_constraint_end(segs.crend())
        , m_line_begin(0)
        , m_line_end(static_cast<int>(line_length))
    {
    }

    void reduce_alternative(LineSpanW& reduced_line, const LineSpanW& alternative)
    {
        assert(reduced_line.size() == alternative.size());
        const int line_sz = static_cast<int>(reduced_line.size());
        for (int idx = 0; idx < line_sz; idx++)
        {
            auto& reduced_tile = reduced_line[idx];
            if (reduced_tile != Tile::UNKNOWN && reduced_tile != alternative[idx])
                reduced_tile = Tile::UNKNOWN;
        }
    }

    struct ReducedLine
    {
    public:
        ReducedLine(LineSpanW& line_span)
            : m_line(line_span)
            , m_reset(true)
        {}

        void reduce(const LineSpanW& alternative)
        {
            assert(m_line.size() == alternative.size());
            if (m_reset)
            {
                m_reset = false;
                copy_line_span(m_line, alternative);
            }
            else
            {
                reduce_alternative(m_line, alternative);
            }
        }

    private:
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
    }

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

    // An array of LineExt
    class LineExtArray
    {
    public:
        LineExtArray(const LineSpan& line_span, std::size_t count, Tile init_tile)
            : m_tiles(count * (line_span.size() + 1u) + 1u, init_tile)
            , m_line_spans()
        {
            m_tiles[0] = Tile::EMPTY;
            for (std::size_t c = 1u; c <= count; c++)
                m_tiles[c * (line_span.size() + 1u)] = Tile::EMPTY;
            m_line_spans.reserve(count);
            for (std::size_t c = 0u; c < count; c++)
                m_line_spans.emplace_back(line_span.type(), line_span.index(), line_span.size(), m_tiles.data() + c * (line_span.size() + 1u) + 1u);
        }

        const LineSpanW& line_span(int idx) const { assert(idx >=0); return m_line_spans[static_cast<unsigned int>(idx)]; }
        LineSpanW& line_span(int idx) {  assert(idx >=0); return m_line_spans[static_cast<unsigned int>(idx)]; }
    private:
        Line::Container         m_tiles;
        std::vector<LineSpanW>  m_line_spans;
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
    result.reserve(static_cast<std::size_t>((line_end - line_begin) / 2));
    bool new_hole = true;
    unsigned int* p_current_length = nullptr;
    for (int index = line_begin; index < line_end; index++)
    {
        const auto& tile = known_tiles[index];
        if (tile == Tile::EMPTY)
        {
            new_hole = true;
            continue;
        }
        if (new_hole)
        {
            p_current_length = &(result.emplace_back(index).m_length);
            new_hole = false;
        }
        assert(p_current_length != nullptr);
        (*p_current_length)++;
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
        return std::make_pair(success, std::move(result));
    }

    struct TailReduce
    {
        TailReduce(LineSpanW tail, LineAlternatives::NbAlt nb_alt = 0)
            : m_reduced_tail(tail)
            , m_nb_alt(nb_alt)
        {}

        LineSpanW               m_reduced_tail;
        LineAlternatives::NbAlt m_nb_alt;
    };

    unsigned int arithmetic_sum_up_to(unsigned int n)
    {
        return (n * (n + 1)) / 2;
    }

    class TailReduceArray
    {
    public:
        // Size required by the line buffer used for the full reduction (it is in O(k.n^2))
        static std::size_t reduced_lines_buffer_size(unsigned int max_k, unsigned int max_line_length)
        {
            return max_k * arithmetic_sum_up_to(max_line_length);
        }
    public:
        TailReduceArray(Line::Type type, Line::Index index, unsigned int max_k, unsigned int max_line_length, FullReductionBuffers& buffers)
            : m_all_reduced_lines(type, index, reduced_lines_buffer_size(max_k, max_line_length), buffers.m_line_buffer.begin())
            , m_nb_alternatives(buffers.m_alts_buffer.data(), max_k * max_line_length)
            , m_recorded(buffers.m_bool_buffer.data(), max_k * max_line_length)
            , m_max_line_length(max_line_length)
        {
            assert(buffers.m_line_buffer.size() >= reduced_lines_buffer_size(max_k, max_line_length));
            for (auto& alt : m_nb_alternatives) { alt = LineAlternatives::NbAlt{0}; }
            for (auto& r : m_recorded) { r = 0; }
        }

        TailReduce tail_reduce(unsigned int k, int n)
        {
            assert(0 <= n && n < static_cast<int>(m_max_line_length));
            const auto uns_n = static_cast<unsigned int>(n);
            unsigned int offset = k * arithmetic_sum_up_to(m_max_line_length) + uns_n * m_max_line_length + uns_n - arithmetic_sum_up_to(uns_n);
            assert(offset < m_all_reduced_lines.size());
            return TailReduce(
                        LineSpanW(m_all_reduced_lines.type(), m_all_reduced_lines.index(), m_max_line_length - uns_n, m_all_reduced_lines.begin() + offset),
                        nb_alternatives(k, uns_n));
        }

        void record_reduction(unsigned int k, int n, LineAlternatives::NbAlt nb_alt)
        {
            assert(0 <= n && n < static_cast<int>(m_max_line_length));
            const auto uns_n = static_cast<unsigned int>(n);
            nb_alternatives(k, uns_n) = nb_alt;
            recorded(k, uns_n) = 1;
        }

        bool is_recorded(unsigned int k, int n) const
        {
            assert(0 <= n && n < static_cast<int>(m_max_line_length));
            const auto uns_n = static_cast<unsigned int>(n);
            return const_cast<TailReduceArray*>(this)->recorded(k, uns_n) != 0;
        }

    private:
        LineAlternatives::NbAlt& nb_alternatives(unsigned int k, unsigned int n)
        {
            assert(n < m_max_line_length);
            assert((k * m_max_line_length + n) < m_nb_alternatives.size());
            return m_nb_alternatives[k * m_max_line_length + n];
        }

        char& recorded(unsigned int k, unsigned int n)
        {
            assert(n < m_max_line_length);
            assert((k * m_max_line_length + n) < m_recorded.size());
            return m_recorded[k * m_max_line_length + n];
        }

    private:
        LineSpanW                               m_all_reduced_lines;
        stdutils::span<LineAlternatives::NbAlt> m_nb_alternatives;
        stdutils::span<char>                    m_recorded;
        unsigned int                            m_max_line_length;
    };

    bool check_ref_line_compatibility_bw_segment(const LineSpanW& ref_line, int segment_begin_index, unsigned int segment_length)
    {
        const int segment_end_index = segment_begin_index + static_cast<int>(segment_length);
        if (ref_line[segment_begin_index - 1] == Tile::FILLED || ref_line[segment_end_index] == Tile::FILLED)
            return false;
        for (int idx = segment_begin_index; idx < segment_end_index; idx++)
            if (ref_line[idx] == Tile::EMPTY)
                return false;
        return true;
    }

} // namespace


struct LineAlternatives::Impl
{
    Impl(const LineConstraint& constraint, const LineSpan& known_tiles, BinomialCoefficients::Cache& binomial);
    Impl(const Impl& other, const LineSpan& known_tiles);

    void reset();

    template <bool Reversed>
    bool check_compatibility_bw(const LineSpanW& alternative, int start_idx, int end_idx) const;

    bool check_compatibility_bw_segment(int segment_begin_index, unsigned int segment_length) const;

    unsigned int nb_unknown_tiles() const;

    template <bool Reversed>
    BidirectionalRange<Reversed>& bidirectional_range();

    template <bool Reversed>
    bool update_range();

    bool update();

    void next_segment_max_index(int& next_segment_index, unsigned int segment_length, int max_segment_index) const;
    void prev_segment_min_index(int& prev_segment_index, unsigned int prev_segment_length, int min_segment_index) const;

    bool narrow_down_segments_range(std::vector<SegmentRange>& ranges) const;

    LineAlternatives::Reduction linear_reduction(const std::vector<SegmentRange>& ranges);

    TailReduce reduce_all_alternatives_recursive(
        LineSpanW& alternative,
        TailReduceArray& tail_reduce_array,
        const int remaining_zeros,
        const Segments::const_iterator constraint_it,
        const Segments::const_iterator constraint_end,
        const int line_begin,
        const int line_end);

    Reduction reduce_all_alternatives(FullReductionBuffers* buffers = nullptr);

    const Segments&                     m_segments;
    const LineSpan                      m_known_tiles;
    LineExt                             m_known_tiles_extended_copy;
    const LineSpanW                     m_known_tiles_ext;
    BinomialCoefficients::Cache&        m_binomial;
    const unsigned int                  m_line_length;
    unsigned int                        m_remaining_zeros;
    BidirectionalRange<false>           m_bidirectional_range;
    BidirectionalRange<true>            m_bidirectional_range_reverse;
};

LineAlternatives::Impl::Impl(const LineConstraint& constraints, const LineSpan& known_tiles, BinomialCoefficients::Cache& binomial)
    : m_segments(constraints.segments())
    , m_known_tiles(known_tiles)
    , m_known_tiles_extended_copy(known_tiles)
    , m_known_tiles_ext(m_known_tiles_extended_copy.line_span())
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
    , m_known_tiles_extended_copy(known_tiles)
    , m_known_tiles_ext(m_known_tiles_extended_copy.line_span())
    , m_binomial(other.m_binomial)
    , m_line_length(other.m_line_length)
    , m_remaining_zeros(other.m_remaining_zeros)
    , m_bidirectional_range(other.m_bidirectional_range)
    , m_bidirectional_range_reverse(other.m_bidirectional_range_reverse)
{
    assert(m_line_length == m_known_tiles.size());
}

void LineAlternatives::Impl::reset()
{
    m_remaining_zeros = m_line_length - compute_min_line_size(m_segments);
    m_bidirectional_range = BidirectionalRange<false>(m_segments, m_line_length);
    m_bidirectional_range_reverse = BidirectionalRange<true>(m_segments, m_line_length);
}

template <bool Reversed>
bool LineAlternatives::Impl::check_compatibility_bw(const LineSpanW& alternative, int start_idx, int end_idx) const
{
    static_assert(static_cast<TileImpl>(Tile::UNKNOWN) == 0);
    static constexpr TileImpl INCOMPATIBLE_SUM = static_cast<TileImpl>(Tile::EMPTY) + static_cast<TileImpl>(Tile::FILLED);
    assert(start_idx <= end_idx);
    for (int idx = start_idx; idx < end_idx; ++idx)
    {
        const auto trans_idx = IndexTranslation<Reversed>()(m_line_length, idx);
        if (static_cast<TileImpl>(m_known_tiles[trans_idx]) + static_cast<TileImpl>(alternative[trans_idx]) == INCOMPATIBLE_SUM)
            return false;
    }
    return true;
}

// Returns true if the segment matches the current known tiles
bool LineAlternatives::Impl::check_compatibility_bw_segment(int segment_begin_index, unsigned int segment_length) const
{
    return check_ref_line_compatibility_bw_segment(m_known_tiles_ext, segment_begin_index, segment_length);
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
                range.m_constraint_begin++;
            }
            range.m_line_begin = line_idx + 1;
        }
        else
        {
            assert(tile == Tile::FILLED);
            if (range.m_constraint_begin == range.m_constraint_end)
                return false;   // Did not expect another segment
            expect_termination_zero = true;
            count_filled++;
        }
    }
    if (expect_termination_zero)
    {
        assert(range.m_constraint_begin != range.m_constraint_end);
        if (count_filled > *range.m_constraint_begin)
            return false;
    }
    assert(range.m_line_begin <= range.m_line_end);
    return true;
}

bool LineAlternatives::Impl::update()
{
    // Update the extended copy of the known tiles
    copy_line_span(m_known_tiles_extended_copy.line_span(), m_known_tiles);

    auto& range_l = bidirectional_range<false>();
    auto& range_r = bidirectional_range<true>();

    const bool valid_l = update_range<false>();
    if (!valid_l)
        return false;

    range_r.m_line_end = static_cast<int>(m_line_length) - range_l.m_line_begin;
    assert(range_r.m_line_begin <= range_r.m_line_end);
    range_r.m_constraint_end = std::make_reverse_iterator(range_l.m_constraint_begin);

    const bool valid_r = update_range<true>();
    if (!valid_r)
        return false;

    range_l.m_line_end = static_cast<int>(m_line_length) - range_r.m_line_begin;
    range_l.m_constraint_end = range_r.m_constraint_begin.base();

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

void LineAlternatives::Impl::next_segment_max_index(int& next_segment_index, unsigned int segment_length, int max_segment_index) const
{
    const int idx_end = next_segment_index;
    for (int idx = max_segment_index + static_cast<int>(segment_length); idx <= idx_end; idx++)
    {
        if (m_known_tiles[idx] == Tile::FILLED)
        {
            next_segment_index = idx;
            break;
        }
    }
}

void LineAlternatives::Impl::prev_segment_min_index(int& prev_segment_index, unsigned int prev_segment_length, int min_segment_index) const
{
    assert(prev_segment_length >= 1);
    const auto idx_end = prev_segment_index;
    for (int idx = min_segment_index - 1; idx >= idx_end; idx--)
    {
        if (m_known_tiles[idx] == Tile::FILLED)
        {
            prev_segment_index = std::max(idx - static_cast<int>(prev_segment_length) + 1, prev_segment_index);
            break;
        }
    }
}

bool LineAlternatives::Impl::narrow_down_segments_range(std::vector<SegmentRange>& ranges) const
{
    const auto nb_segments = ranges.size();
    const auto constraint_begin = m_bidirectional_range.m_constraint_begin;
    const auto constraint_end = m_bidirectional_range.m_constraint_end;

    // Left to right pass to refine the rightmost index of each segment
    if (nb_segments > 0)
        next_segment_max_index(ranges[0].m_rightmost_index, 0, m_bidirectional_range.m_line_begin);
    auto constraint_it = constraint_begin;
    for (std::size_t k = 0; k + 1u < nb_segments; k++)
    {
        assert(constraint_it != constraint_end);
        int max_index = -1;
        const auto seg_length = *constraint_it;
        for (int seg_index = ranges[k].m_rightmost_index; seg_index >= ranges[k].m_leftmost_index; seg_index--)
        {
            if (check_compatibility_bw_segment(seg_index, seg_length))
            {
                max_index = seg_index;
                break;
            }
        }
        if (max_index >= 0)
            next_segment_max_index(ranges[k+1].m_rightmost_index, seg_length, max_index);
        constraint_it++;
    }

    // Right to left pass to refine the leftmost index of each segment
    if (nb_segments > 0)
    {
        assert(constraint_it != constraint_end && std::next(constraint_it) == constraint_end);
        prev_segment_min_index(ranges[nb_segments - 1].m_leftmost_index, *constraint_it, m_bidirectional_range.m_line_end);
        for (std::size_t k = nb_segments - 1; k > 0; k--)
        {
            assert(std::distance(constraint_begin, constraint_it) == static_cast<std::ptrdiff_t>(k));
            int min_index = -1;
            const auto seg_length = *constraint_it;
            for (int seg_index = ranges[k].m_leftmost_index; seg_index <= ranges[k].m_rightmost_index; seg_index++)
            {
                if (check_compatibility_bw_segment(seg_index, seg_length))
                {
                    min_index = seg_index;
                    break;
                }
            }
            assert(constraint_it != constraint_begin);
            if (min_index >= 0)
                prev_segment_min_index(ranges[k-1].m_leftmost_index, *std::prev(constraint_it), min_index);
            constraint_it--;
        }
    }

    // Check consistency of the ranges and return
    return std::all_of(ranges.cbegin(), ranges.cend(), [](const auto& range) { return range.m_leftmost_index <= range.m_rightmost_index; });
}

// Linear reduction considers each segment independently, therefore leading to a O(k.n) reduction, where k is the number of segment and n is the line length
LineAlternatives::Reduction LineAlternatives::Impl::linear_reduction(const std::vector<SegmentRange>& ranges)
{
    assert(m_known_tiles == LineSpan(m_known_tiles_ext));     // Assert that update() was called

    const auto nb_segments = ranges.size();
    auto constraint_it = m_bidirectional_range.m_constraint_begin;
    const auto constraint_end = m_bidirectional_range.m_constraint_end;
    const auto line_begin =  m_bidirectional_range.m_line_begin;
    const auto line_end = m_bidirectional_range.m_line_end;
    assert(nb_segments == static_cast<std::size_t>(std::distance(constraint_it, constraint_end)));

    LineExtArray tiles_masks(m_known_tiles, 1u, Tile::UNKNOWN);

    // empty_tiles_mask:  '0??000000?????0'
    LineSpanW& empty_tiles_mask = tiles_masks.line_span(0);
    for (int idx = 0; idx < line_begin; idx++) { empty_tiles_mask[idx] = Tile::UNKNOWN; }
    for (int idx = line_begin; idx < line_end; idx++) { empty_tiles_mask[idx] = Tile::EMPTY; }
    for (int idx = line_end; idx < static_cast<int>(m_line_length); idx++) { empty_tiles_mask[idx] = Tile::UNKNOWN; }

    LineExt reduction_mask_extended_line(m_known_tiles);
    LineSpanW& reduction_mask = reduction_mask_extended_line.line_span();
    assert(reduction_mask == m_known_tiles_ext);

    Reduction result = from_line(m_known_tiles, 1, false);
    LineSpanW reduced_line(result.reduced_line);
    for (std::size_t k = 0; k < nb_segments; k++)
    {
        NbAlt nb_alt = 0u;
        int min_index = static_cast<int>(m_known_tiles.size() + 1u);
        int max_index = -1;
        assert(constraint_it != constraint_end);
        const unsigned int seg_length = *constraint_it;
        for (int seg_index = ranges[k].m_leftmost_index; seg_index <= ranges[k].m_rightmost_index; seg_index++)
        {
            if (check_ref_line_compatibility_bw_segment(reduction_mask, seg_index, seg_length))
            {
                nb_alt++;
                min_index = std::min(min_index, seg_index);
                max_index = std::max(max_index, seg_index);
                const int seg_index_end = seg_index + static_cast<int>(seg_length);
                for (int idx = seg_index; idx < seg_index_end; idx++)
                {
                    // Empty tiles mask
                    empty_tiles_mask[idx] = Tile::UNKNOWN;
                }
            }
        }
        if (nb_alt == 1)
        {
            assert(min_index == max_index);
            reduction_mask[min_index - 1] = Tile::EMPTY;
            reduction_mask[min_index + static_cast<int>(seg_length)] = Tile::EMPTY;
        }
        if (nb_alt > 0)
        {
            for (int idx = max_index; idx < min_index + static_cast<int>(seg_length); idx++)
                reduction_mask[idx] = Tile::FILLED;
        }
        BinomialCoefficients::mult(result.nb_alternatives, nb_alt);
        if (result.nb_alternatives == 0)
            break;
        constraint_it++;
    }

    reduced_line += reduction_mask;

    // Empty tiles mask
    if (are_compatible(m_known_tiles, LineSpan(empty_tiles_mask)))
        reduced_line += LineSpan(empty_tiles_mask);
    else
        result.nb_alternatives = 0;

    // Compute over estimate of the nb of alternatives
    if (result.nb_alternatives > 0 && nb_segments > 0)
    {
        const int nb_zeros = static_cast<int>(std::count_if(reduced_line.begin() + line_begin, reduced_line.begin() + line_end, [](const Tile& tile) { return tile == Tile::EMPTY; }));
        const int remaining_zeros = static_cast<int>(m_remaining_zeros) - std::max(0, nb_zeros - static_cast<int>(nb_segments - 1));
        if (remaining_zeros < 0)
        {
            result.nb_alternatives = 0;
        }
        else if (remaining_zeros == 0)
        {
            for (int idx = line_begin; idx < line_end; idx++)
            {
                assert(idx >= 0);
                auto& tile = reduced_line[idx];
                if (tile == Tile::UNKNOWN)
                    tile = Tile::FILLED;
            }
            result.nb_alternatives = 1;
            assert(result.reduced_line.is_completed());
        }
        else
        {
            assert(remaining_zeros > 0);
            result.nb_alternatives = std::min(result.nb_alternatives, compute_max_nb_of_alternatives(static_cast<unsigned int>(remaining_zeros), static_cast<unsigned int>(nb_segments), m_binomial));
        }
    }

    result.is_fully_reduced = (result.nb_alternatives == 1);

    return result;
}

TailReduce LineAlternatives::Impl::reduce_all_alternatives_recursive(
    LineSpanW& alternative,
    TailReduceArray& tail_reduce_array,
    const int remaining_zeros,
    const Segments::const_iterator constraint_it,
    const Segments::const_iterator constraint_end,
    const int line_begin,
    const int line_end)
{
    static_assert(std::is_same_v<NbAlt, BinomialCoefficients::Rep>);
    assert(constraint_it != constraint_end);
    const auto k = static_cast<unsigned int>(std::distance(constraint_it, constraint_end)) - 1;

    if (line_begin == line_end)
    {
        // Always query with n = 0 to avoid the case n = line_length
        TailReduce invalid_result = tail_reduce_array.tail_reduce(k, 0);
        invalid_result.m_nb_alt = 0;
        return invalid_result;
    }
    assert(line_begin < line_end);

    TailReduce result = tail_reduce_array.tail_reduce(k, line_begin);
    if (tail_reduce_array.is_recorded(k, line_begin))
    {
        // The intermediate reduction was previously memoized
        return result;
    }

    // Compute reduction
    assert(result.m_nb_alt == 0);
    auto reduced_tail = result.m_reduced_tail.head(line_end - line_begin);
    ReducedLine reduced_line_tail(reduced_tail);
    LineSpanW alternative_tail = alternative.head(line_end).tail(line_begin);
    assert(alternative_tail.size() == static_cast<std::size_t>(line_end - line_begin));

    const int nb_ones = static_cast<int>(*constraint_it);
    const bool is_last_constraint = (constraint_it + 1 == constraint_end);
    for (int idx = 0; idx < nb_ones; idx++)
    {
        alternative_tail[idx] = Tile::FILLED;
    }
    if (is_last_constraint)
    {
        assert(line_begin + nb_ones + remaining_zeros == line_end);
        for (int idx = nb_ones; idx < (nb_ones + remaining_zeros); idx++)
        {
            alternative_tail[idx] = Tile::EMPTY;
        }
        NbAlt nb_alt = 0;
        for (int pre_zeros = 0; pre_zeros <= remaining_zeros; pre_zeros++)
        {
            int next_line_idx = pre_zeros - 1;
            alternative_tail[next_line_idx] = Tile::EMPTY;
            next_line_idx += nb_ones;
            alternative_tail[next_line_idx++] = Tile::FILLED;
            if (check_compatibility_bw<false>(alternative, line_begin, line_end))
            {
                reduced_line_tail.reduce(alternative_tail);
                nb_alt++;
            }
        }
        BinomialCoefficients::add(result.m_nb_alt, nb_alt);
    }
    else
    {
        for (int pre_zeros = 0; pre_zeros <= remaining_zeros; pre_zeros++)
        {
            int next_line_idx = pre_zeros - 1;
            alternative_tail[next_line_idx] = Tile::EMPTY;
            next_line_idx += nb_ones;
            alternative_tail[next_line_idx++] = Tile::FILLED;
            alternative_tail[next_line_idx++] = Tile::EMPTY;        // Terminating empty tile
            // TODO: The limiting factor for performance is probably here: O(n) calls to a linear function check_compatibility_bw.
            if (check_compatibility_bw<false>(alternative, line_begin, line_begin + next_line_idx))
            {
                assert(line_begin + next_line_idx <= line_end);
                const int tail_sz = line_end - line_begin - next_line_idx;
                const auto recurse_tail_reduce = reduce_all_alternatives_recursive(alternative, tail_reduce_array, remaining_zeros - pre_zeros, constraint_it + 1, constraint_end, line_begin + next_line_idx, line_end);
                if (recurse_tail_reduce.m_nb_alt > 0)
                {
                    for (int idx = 0; idx < tail_sz; idx++)
                    {
                        alternative_tail[next_line_idx + idx] = recurse_tail_reduce.m_reduced_tail[idx];
                    }
                    reduced_line_tail.reduce(alternative_tail);
                    BinomialCoefficients::add(result.m_nb_alt, recurse_tail_reduce.m_nb_alt);
                }
            }
        }
    }

    // Memoization of this intermediate reduction
    tail_reduce_array.record_reduction(k, line_begin, result.m_nb_alt);

    return result;
}

// Line solver that performs a full reduction: all alternatives are theoritically explored and "reduced" to a line which contains all the tiles
// that can be deduced from the input contraints and known tiles. In addition, the alogrithm returns the total number of alternative solutions.
// With:
//  k = numbers of constraints
//  n = length of the line
// Time complexity :   O(k.n^3)   (can surely be brought down to O(k.n^2))
// Memory complexity : O(k.n^2)   (the quadratic size memory buffer is freed at the end of the full_reduction)
LineAlternatives::Reduction LineAlternatives::Impl::reduce_all_alternatives(FullReductionBuffers* buffers)
{
    const auto& range_l = m_bidirectional_range;
    Line reduced_line_raw = line_from_line_span(m_known_tiles);
    LineSpanW reduced_line(reduced_line_raw);
    if ((range_l.m_constraint_begin == range_l.m_constraint_end) || (range_l.m_line_begin == range_l.m_line_end))
    {
        for (int idx = range_l.m_line_begin; idx < range_l.m_line_end; idx++)
        {
            reduced_line[idx] = Tile::EMPTY;
        }
        const bool match = check_compatibility_bw<false>(reduced_line, range_l.m_line_begin, range_l.m_line_end);
        return Reduction { std::move(reduced_line_raw), match ? NbAlt{1} : NbAlt{0}, true };
    }
    else
    {
        assert(range_l.m_line_begin < range_l.m_line_end);
        LineExt alternative_buffer(m_known_tiles);
        const auto k = static_cast<unsigned int>(std::distance(range_l.m_constraint_begin, range_l.m_constraint_end));
        std::unique_ptr<FullReductionBuffers> reduction_buffers;
        if (!buffers)
        {
            reduction_buffers = std::make_unique<FullReductionBuffers>(k, m_line_length);
            buffers = reduction_buffers.get();
        }
        assert(buffers);
        TailReduceArray tail_reduce_array(m_known_tiles.type(), m_known_tiles.index(), k, m_line_length, *buffers);
        const auto reduction = reduce_all_alternatives_recursive(alternative_buffer.line_span(), tail_reduce_array, static_cast<int>(m_remaining_zeros), range_l.m_constraint_begin, range_l.m_constraint_end, range_l.m_line_begin, range_l.m_line_end);
        for (int idx = range_l.m_line_begin; idx < range_l.m_line_end; idx++)
        {
            reduced_line[idx] = reduction.m_reduced_tail[idx - range_l.m_line_begin];
        }
        return Reduction { std::move(reduced_line_raw), reduction.m_nb_alt, true };
    }
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

void LineAlternatives::reset()
{
    p_impl->reset();
}

LineAlternatives::Reduction LineAlternatives::full_reduction(FullReductionBuffers* buffers)
{
    // Update extended copy of known tiles and bidirectional ranges
    bool valid = p_impl->update();
    if (!valid)
        return from_line(p_impl->m_known_tiles, 0, false);

    return p_impl->reduce_all_alternatives(buffers);
}

LineAlternatives::Reduction LineAlternatives::linear_reduction()
{
    const LineSpan& known_tiles = p_impl->m_known_tiles;
    const auto invalid_result = [](const LineSpan& line) -> Reduction { return from_line(line, 0, false); };

    // Update extended copy of known tiles and bidirectional ranges
    bool valid = p_impl->update();
    if (!valid)
        return invalid_result(known_tiles);

    // Compute the leftmost and rightmost position of each segment
    std::vector<SegmentRange> ranges;
    std::tie(valid, ranges) = local_find_segments_range(known_tiles, p_impl->m_bidirectional_range);
    if (!valid)
        return invalid_result(known_tiles);

    // Narrow down the leftmost and rightmost ranges
    valid = p_impl->narrow_down_segments_range(ranges);
    if (!valid)
        return invalid_result(known_tiles);

    // Compute the linear reduction
    return p_impl->linear_reduction(ranges);
}

// For testing purpose
std::pair<bool, std::vector<SegmentRange>> LineAlternatives::find_segments_range() const
{
    BidirectionalRange<false> range(p_impl->m_segments, p_impl->m_line_length);
    return local_find_segments_range(p_impl->m_known_tiles, range);
}

FullReductionBuffers::FullReductionBuffers(unsigned int max_k, unsigned int max_line_length)
    : m_line_buffer(Line::ROW, 0, TailReduceArray::reduced_lines_buffer_size(max_k, max_line_length), Tile::UNKNOWN)
    , m_alts_buffer(max_k * max_line_length, LineAlternatives::NbAlt{0})
    , m_bool_buffer(max_k * max_line_length, 0)
{}

} // namespace picross
