#pragma once

#include <picross/picross.h>

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <vector>

namespace picross
{
namespace BinomialCoefficients { class Cache; }
class LineConstraint;
class LineSpan;

struct LineHole
{
    int          m_index;
    unsigned int m_length;
};
inline bool operator==(const LineHole& lhs, const LineHole& rhs) { return lhs.m_index == rhs.m_index && lhs.m_length == rhs.m_length; }
std::ostream& operator<<(std::ostream& out, const LineHole& line_hole);

std::vector<LineHole> line_holes(const LineSpan& known_tiles, int line_begin = 0, int line_end = -1);

struct SegmentRange
{
    int m_leftmost_index;
    int m_rightmost_index;
};
inline bool operator==(const SegmentRange& lhs, const SegmentRange& rhs) { return lhs.m_leftmost_index == rhs.m_leftmost_index && lhs.m_rightmost_index == rhs.m_rightmost_index; }
std::ostream& operator<<(std::ostream& out, const SegmentRange& segment_range);


// Given a constraint, recursively build the possible alternatives of a Line and reduce them.
class LineAlternatives
{
public:
    using NbAlt = std::uint32_t;
public:
    LineAlternatives(const LineConstraint& constraint, const LineSpan& known_tiles, BinomialCoefficients::Cache& binomial);
    LineAlternatives(const LineAlternatives& other, const LineSpan& known_tiles);
    ~LineAlternatives();
    // Movable
    LineAlternatives(LineAlternatives&&) noexcept;
    LineAlternatives& operator=(LineAlternatives&&) noexcept;
public:
    // Regarding the reduction result:
    //  - If nb_alternatives == 0, the line (and therefore, the grid) is contradictory
    //  - If is_fully_reduced == true, all alternatives were computed and reduced. The value of nb_alternatives is exact
    //  - If is_fully_reduced == false, the value of nb_alternatives is an estimate (most likely, it is overestimated)
    struct Reduction
    {
        Line reduced_line = Line(Line::ROW, 0, 0);
        NbAlt nb_alternatives = 0;
        bool is_fully_reduced = false;
    };
    Reduction full_reduction();
    Reduction partial_reduction(unsigned int nb_constraints);
    Reduction linear_reduction();

    // For test purpose only
    std::pair<bool, std::vector<SegmentRange>> find_segments_range() const;

private:
    struct Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace picross
