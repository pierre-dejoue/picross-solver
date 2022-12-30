/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#include "line_constraint.h"

#include "line.h"
#include "line_alternatives.h"

#include <algorithm>
#include <cassert>


namespace picross
{

unsigned int compute_min_line_size(const Segments& segments)
{
    return compute_min_line_size(segments.cbegin(), segments.cend());
}

LineConstraint::LineConstraint(Line::Type type, const InputGrid::Constraint& vect)
    : m_type(type)
    , m_segments()
    , m_min_line_size(0u)
{
    // Filter out segments of length zero
    m_segments.reserve(vect.size());
    std::copy_if(vect.cbegin(), vect.cend(), std::back_inserter(m_segments), [](const auto c) { return c > 0; });

    if (m_segments.size() != 0u)
    {
        // Include at least one zero between the sets of one
        m_min_line_size = compute_min_line_size(m_segments);
    }
}

unsigned int LineConstraint::nb_filled_tiles() const
{
    return std::accumulate(m_segments.cbegin(), m_segments.cend(), 0u);
}

unsigned int LineConstraint::max_segment_size() const
{
    return m_segments.empty() ? 0u : *max_element(m_segments.cbegin(), m_segments.cend());
}

// Given an uninitialized line compute the theoritical number of alternatives
unsigned int LineConstraint::line_trivial_nb_alternatives(unsigned int line_size, BinomialCoefficients::Cache& binomial_cache) const
{
    if (line_size < m_min_line_size)
    {
        // This can happen if the user does not check the grid with check_input_grid()
        throw std::logic_error("Constraint::line_trivial_reduction: line_size < min_line_size");
    }

    const unsigned int nb_zeros = line_size - m_min_line_size;
    const auto nb_alternatives = binomial_cache.partition_n_elts_into_k_buckets(nb_zeros, static_cast<unsigned int>(m_segments.size()) + 1u);
    assert(nb_alternatives > 0u);
    return nb_alternatives;
}

// Given an uninitialized line compute the trivial reduction
Line LineConstraint::line_trivial_reduction(unsigned int line_size, unsigned int index) const
{
    Line line(m_type, index, line_size);

    if (line_size < m_min_line_size)
    {
        // This can happen if the user does not check the grid with check_input_grid()
        throw std::logic_error("Constraint::line_trivial_reduction: line_size < min_line_size");
    }
    const unsigned int nb_zeros = line_size - m_min_line_size;

    Line::Container& tiles = line.tiles();
    unsigned int line_idx = 0u;

    const auto max_seg_sz = max_segment_size();
    if (max_seg_sz == 0u)
    {
        // Blank line
        for (unsigned int c = 0u; c < line_size; c++)
        {
            tiles[line_idx++] = Tile::EMPTY;
        }
        assert(line_idx == line_size);
        assert(is_complete(line));
    }
    else if (nb_zeros == 0u)
    {
        // The line is fully defined
        for (unsigned int seg_idx = 0u; seg_idx < m_segments.size(); seg_idx++)
        {
            const auto seg_sz = m_segments[seg_idx];
            const bool last = (seg_idx + 1u == m_segments.size());
            for (unsigned int c = 0u; c < seg_sz; c++) { tiles[line_idx++] = Tile::FILLED; }
            if (!last) { tiles[line_idx++] = Tile::EMPTY; }
        }
        assert(line_idx == line_size);
        assert(is_complete(line));
    }
    else if (max_seg_sz > nb_zeros)
    {
        // The reduction is straightforward in that case
        for (unsigned int seg_idx = 0u; seg_idx < m_segments.size(); seg_idx++)
        {
            const auto seg_sz = m_segments[seg_idx];
            assert(seg_sz > 0);
            const bool last = (seg_idx + 1u == m_segments.size());
            for (unsigned int c = 0u; c < seg_sz; c++) { if (c >= nb_zeros) { tiles[line_idx] = Tile::FILLED; }; line_idx++; }
            if (!last) { line_idx++; /* would be Tile::EMPTY */ }
        }
        assert(line_idx + nb_zeros == line_size);
        assert(!is_all_one_color(line, Tile::UNKNOWN));
    }

    return line;
}

std::vector<Line> LineConstraint::build_all_possible_lines(const Line& known_tiles) const
{
    assert(known_tiles.type() == m_type);
    const auto index = known_tiles.index();

    // Number of zeros to add to the minimal size line.
    assert(known_tiles.size() >= m_min_line_size);
    unsigned int nb_zeros = static_cast<unsigned int>(known_tiles.size()) - m_min_line_size;

    std::vector<Line> result;
    Line new_line(known_tiles, Tile::UNKNOWN);
    Line::Container& new_tile_vect = new_line.tiles();

    if (m_segments.size() == 0)
    {
        // Return a list with only one all-zero line
        unsigned int line_idx = 0u;
        for (unsigned int c = 0u; c < known_tiles.size(); c++) { new_tile_vect[line_idx++] = Tile::EMPTY; }

        // Filter against known_tiles
        if (new_line.compatible(known_tiles))
        {
            result.emplace_back(m_type, index, new_tile_vect);
        }
    }
    else if (m_segments.size() == 1)
    {
        // Build the list of all possible lines with only one block of continuous filled tiles.
        // NB: in this case nb_zeros = size - m_segments[0]
        for (unsigned int n = 0u; n <= nb_zeros; n++)
        {
            unsigned int line_idx = 0u;
            for (unsigned int c = 0u; c < n; c++) { new_tile_vect[line_idx++] = Tile::EMPTY; }
            for (unsigned int c = 0u; c < m_segments[0]; c++) { new_tile_vect[line_idx++] = Tile::FILLED; }
            while (line_idx < known_tiles.size()) { new_tile_vect[line_idx++] = Tile::EMPTY; }

            // Filter against known_tiles
            if (new_line.compatible(known_tiles))
            {
                result.emplace_back(m_type, index, new_tile_vect);
            }
        }
    }
    else
    {
        // For loop on the number of zeros before the first block of ones
        for (unsigned int n = 0u; n <= nb_zeros; n++)
        {
            unsigned int line_idx = 0u;
            for (unsigned int c = 0u; c < n; c++) { new_tile_vect[line_idx++] = Tile::EMPTY; }
            for (unsigned int c = 0u; c < m_segments[0]; c++) { new_tile_vect[line_idx++] = Tile::FILLED; }
            new_tile_vect[line_idx++] = Tile::EMPTY;

            // Filter against known_tiles
            if (new_line.compatible(known_tiles))
            {
                // If OK, then go on and recursively call this function to construct the remaining part of the line.
                std::vector<unsigned int> trim_sets_of_ones(m_segments.begin() + 1, m_segments.end());
                LineConstraint recursive_constraint(m_type, trim_sets_of_ones);

                std::vector<Tile> end_known_vect(known_tiles.tiles().cbegin() + line_idx, known_tiles.tiles().cend());
                Line end_known_tiles(m_type, index, std::move(end_known_vect));

                std::vector<Line> recursive_list = recursive_constraint.build_all_possible_lines(end_known_tiles);

                // Finally, construct the return_list based on the contents of the recursive_list.
                for (const Line& line : recursive_list)
                {
                    std::copy(line.tiles().cbegin(), line.tiles().cend(), new_tile_vect.begin() + line_idx);
                    result.emplace_back(m_type, index, new_tile_vect);
                }
            }
            else
            {
                // The beginning of the line does not match the known_tiles. Do nothing.
            }
        }
        // Filtering is already done, no need to call add_and_filter_lines()
    }

    return result;
}

bool LineConstraint::compatible(const Line& line) const
{
    assert(is_complete(line));
    const auto segments = get_constraint_from(line);
    return segments == m_segments;
}

void LineConstraint::print(std::ostream& ostream) const
{
    ostream << "Constraint on a " << str_line_type(m_type) << ": [ ";
    for (const auto& seg : m_segments)
    {
        ostream << seg << " ";
    }
    ostream << "]; min_line_size = " << m_min_line_size;
}

std::ostream& operator<<(std::ostream& ostream, const LineConstraint& constraint)
{
    constraint.print(ostream);
    return ostream;
}

} // namespace picross
