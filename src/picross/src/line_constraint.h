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

#include <cstddef>
#include <ostream>
#include <utility>
#include <vector>


namespace picross
{

class LineConstraint
{
public:
    LineConstraint(Line::Type type, const InputGrid::Constraint& vect);
public:
    unsigned int nb_filled_tiles() const;
    std::size_t nb_segments() const { return m_segs_of_ones.size(); }
    const std::vector<unsigned int>& segments() const { return m_segs_of_ones; }
    unsigned int min_line_size() const { return m_min_line_size; }
    unsigned int line_trivial_nb_alternatives(unsigned int line_size, BinomialCoefficientsCache& binomial) const;
    Line line_trivial_reduction(unsigned int line_size, unsigned int index) const;
    std::vector<Line> build_all_possible_lines(const Line& known_tiles) const;
    bool compatible(const Line& line) const;
private:
    unsigned int max_segment_size() const;
    void print(std::ostream& ostream) const;
public:
    friend std::ostream& operator<<(std::ostream& ostream, const LineConstraint& constraint);
private:
    Line::Type                  m_type;                     // Row or column
    std::vector<unsigned int>   m_segs_of_ones;             // Size of the contiguous blocks of filled tiles
    unsigned int                m_min_line_size;            // Minimal line size compatible with this constraint
};

} // namespace picross
