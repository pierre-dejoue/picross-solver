/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PRIVATE API of the Picross solver
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include "binomial.h"

#include <picross/picross.h>

#include <cassert>
#include <iterator>
#include <numeric>
#include <ostream>
#include <utility>
#include <vector>


namespace picross
{
class LineSpan;

using Segments = std::vector<unsigned int>;

template <typename SegmentIt>
unsigned int compute_min_line_size(SegmentIt begin, SegmentIt end)
{
    const auto interval_zeros = (begin == end) ? 0u : static_cast<unsigned int>(std::distance(begin, end)) - 1u;
    return std::accumulate(begin, end, 0u) + interval_zeros;
}

unsigned int compute_min_line_size(const Segments& segments);

class LineConstraint
{
public:
    LineConstraint(Line::Type type, const InputGrid::Constraint& vect);
public:
    unsigned int nb_filled_tiles() const;
    std::size_t nb_segments() const { return m_segments.size(); }
    const Segments& segments() const { return m_segments; }
    unsigned int min_line_size() const { return m_min_line_size; }
    unsigned int line_trivial_nb_alternatives(unsigned int line_size, BinomialCoefficients::Cache& binomial) const;
    Line line_trivial_reduction(unsigned int line_size, unsigned int index) const;
    std::vector<Line> build_all_possible_lines(const LineSpan& known_tiles) const;
    bool compatible(const LineSpan& line) const;
private:
    unsigned int max_segment_size() const;
    void print(std::ostream& ostream) const;
public:
    friend std::ostream& operator<<(std::ostream& ostream, const LineConstraint& constraint);
private:
    Line::Type          m_type;                     // Row or column
    Segments            m_segments;             // Size of the contiguous blocks of filled tiles
    unsigned int        m_min_line_size;            // Minimal line size compatible with this constraint
};

} // namespace picross
