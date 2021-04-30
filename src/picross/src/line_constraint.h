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

#include <ostream>
#include <utility>
#include <vector>


namespace picross
{

/*
 * LineConstraint class
 */
class LineConstraint
{
public:
    LineConstraint(Line::Type type, const InputGrid::Constraint& vect);
public:
    unsigned int nb_filled_tiles() const;                       // Total number of filled tiles
    size_t nb_segments() const { return segs_of_ones.size(); }  // Number of segments of contiguous filled tiles
    unsigned int max_segment_size() const;                      // Max segment size
    unsigned int get_min_line_size() const { return min_line_size; }
    std::pair<bool, unsigned int> line_trivial_reduction(Line& line, BinomialCoefficientsCache& binomial) const;
    std::vector<Line> build_all_possible_lines(const Line& known_tiles) const;
    std::pair<Line, unsigned int> reduce_and_count_alternatives(const Line& filter_line, GridStats * stats) const;
    bool compatible(const Line& line) const;
    void print(std::ostream& ostream) const;
private:
    Line::Type type;                                            // Row or column
    InputGrid::Constraint segs_of_ones;                         // Size of the contiguous blocks of filled tiles
    unsigned int min_line_size;
};


std::ostream& operator<<(std::ostream& ostream, const LineConstraint& constraint);

} // namespace picross
