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
#include <numeric>


namespace picross
{

LineConstraint::LineConstraint(Line::Type type, const InputGrid::Constraint& vect) :
    type(type),
    segs_of_ones()
{
    segs_of_ones.reserve(vect.size());
    std::copy_if(vect.cbegin(), vect.cend(), std::back_inserter(segs_of_ones), [](const auto c) { return c > 0; });
    if (segs_of_ones.size() == 0u)
    {
        min_line_size = 0u;
    }
    else
    {
        /* Include at least one zero between the sets of one */
        min_line_size = std::accumulate(segs_of_ones.cbegin(), segs_of_ones.cend(), 0u) + static_cast<unsigned int>(segs_of_ones.size()) - 1u;
    }
}


unsigned int LineConstraint::nb_filled_tiles() const
{
    return std::accumulate(segs_of_ones.cbegin(), segs_of_ones.cend(), 0u);
}


unsigned int LineConstraint::max_segment_size() const
{
    return segs_of_ones.empty() ? 0u : *max_element(segs_of_ones.cbegin(), segs_of_ones.cend());
}


/*
 * Given an uninitialized line compute the theoritical number of alternatives
 */
unsigned int LineConstraint::line_trivial_nb_alternatives(unsigned int line_size, BinomialCoefficientsCache& binomial_cache) const
{
    if (line_size < min_line_size)
    {
        // This can happen if the user does not check the grid with check_grid_input()
        throw std::logic_error("Constraint::line_trivial_reduction: line_size < min_line_size");
    }

    const unsigned int nb_zeros = line_size - min_line_size;
    const auto nb_altrnatives = binomial_cache.nb_alternatives_for_fixed_nb_of_partitions(nb_zeros, static_cast<unsigned int>(segs_of_ones.size()) + 1u);
    assert(nb_altrnatives > 0u);
    return nb_altrnatives;
}

/*
 * Given an uninitialized line compute the trivial reduction
 */
Line LineConstraint::line_trivial_reduction(unsigned int line_size, unsigned int index) const
{
    Line line(type, index, line_size);

    if (line_size < min_line_size)
    {
        // This can happen if the user does not check the grid with check_grid_input()
        throw std::logic_error("Constraint::line_trivial_reduction: line_size < min_line_size");
    }
    const unsigned int nb_zeros = line_size - min_line_size;

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
        for (unsigned int seg_idx = 0u; seg_idx < segs_of_ones.size(); seg_idx++)
        {
            const auto seg_sz = segs_of_ones[seg_idx];
            const bool last = (seg_idx + 1u == segs_of_ones.size());
            for (unsigned int c = 0u; c < seg_sz; c++) { tiles[line_idx++] = Tile::FILLED; }
            if (!last) { tiles[line_idx++] = Tile::EMPTY; }
        }
        assert(line_idx == line_size);
        assert(is_complete(line));
    }
    else if (max_seg_sz > nb_zeros)
    {
        // The reduction is straightforward in that case
        for (unsigned int seg_idx = 0u; seg_idx < segs_of_ones.size(); seg_idx++)
        {
            const auto seg_sz = segs_of_ones[seg_idx];
            assert(seg_sz > 0);
            const bool last = (seg_idx + 1u == segs_of_ones.size());
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
    assert(known_tiles.type() == type);
    const size_t index = known_tiles.index();

    // Number of zeros to add to the minimal size line.
    assert(known_tiles.size() >= min_line_size);
    unsigned int nb_zeros = static_cast<unsigned int>(known_tiles.size()) - min_line_size;

    std::vector<Line> result;
    Line new_line(known_tiles, Tile::UNKNOWN);
    Line::Container& new_tile_vect = new_line.tiles();

    if (segs_of_ones.size() == 0)
    {
        // Return a list with only one all-zero line
        unsigned int line_idx = 0u;
        for (unsigned int c = 0u; c < known_tiles.size(); c++) { new_tile_vect[line_idx++] = Tile::EMPTY; }

        // Filter against known_tiles
        if (new_line.compatible(known_tiles))
        {
            result.emplace_back(type, index, new_tile_vect);
        }
    }
    else if (segs_of_ones.size() == 1)
    {
        // Build the list of all possible lines with only one block of continuous filled tiles.
        // NB: in this case nb_zeros = size - segs_of_ones[0]
        for (unsigned int n = 0u; n <= nb_zeros; n++)
        {
            unsigned int line_idx = 0u;
            for (unsigned int c = 0u; c < n; c++) { new_tile_vect[line_idx++] = Tile::EMPTY; }
            for (unsigned int c = 0u; c < segs_of_ones[0]; c++) { new_tile_vect[line_idx++] = Tile::FILLED; }
            while (line_idx < known_tiles.size()) { new_tile_vect[line_idx++] = Tile::EMPTY; }

            // Filter against known_tiles
            if (new_line.compatible(known_tiles))
            {
                result.emplace_back(type, index, new_tile_vect);
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
            for (unsigned int c = 0u; c < segs_of_ones[0]; c++) { new_tile_vect[line_idx++] = Tile::FILLED; }
            new_tile_vect[line_idx++] = Tile::EMPTY;

            // Filter against known_tiles
            if (new_line.compatible(known_tiles))
            {
                // If OK, then go on and recursively call this function to construct the remaining part of the line.
                std::vector<unsigned int> trim_sets_of_ones(segs_of_ones.begin() + 1, segs_of_ones.end());
                LineConstraint recursive_constraint(type, trim_sets_of_ones);

                std::vector<Tile> end_known_vect(known_tiles.tiles().cbegin() + line_idx, known_tiles.tiles().cend());
                Line end_known_tiles(type, index, std::move(end_known_vect));

                std::vector<Line> recursive_list = recursive_constraint.build_all_possible_lines(end_known_tiles);

                // Finally, construct the return_list based on the contents of the recursive_list.
                for (const Line& line : recursive_list)
                {
                    std::copy(line.tiles().cbegin(), line.tiles().cend(), new_tile_vect.begin() + line_idx);
                    result.emplace_back(type, index, new_tile_vect);
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

std::pair<Line, unsigned int> LineConstraint::reduce_and_count_alternatives(const Line& known_tiles, GridStats* stats) const
{
    if (stats != nullptr) { stats->nb_reduce_and_count_alternatives_calls++; }

    assert(known_tiles.type() == type);

    // Number of zeros to add to the minimal size line.
    assert(known_tiles.size() >= min_line_size);
    unsigned int nb_zeros = static_cast<unsigned int>(known_tiles.size()) - min_line_size;

    LineAlternatives builder(segs_of_ones, known_tiles);
    unsigned int nb_alternatives = builder.build_alternatives(nb_zeros);

    return std::make_pair(builder.get_reduced_line(), nb_alternatives);
}

bool LineConstraint::compatible(const Line& line) const
{
    assert(is_complete(line));
    const auto segments = get_constraint_from(line);
    return segments == segs_of_ones;
}


void LineConstraint::print(std::ostream& ostream) const
{
    ostream << "Constraint on a " << str_line_type(type) << ": [ ";
    for (const auto& seg : segs_of_ones)
    {
        ostream << seg << " ";
    }
    ostream << "]; min_line_size = " << min_line_size;
}


std::ostream& operator<<(std::ostream& ostream, const LineConstraint& constraint)
{
    constraint.print(ostream);
    return ostream;
}


InputGrid build_input_grid_from(const OutputGrid& grid)
{
    InputGrid result;

    result.m_name = grid.name();

    result.m_rows.reserve(grid.height());
    for (unsigned int y = 0u; y < grid.height(); y++)
    {
        result.m_rows.emplace_back(get_constraint_from(grid.get_line<Line::ROW>(y)));
    }

    result.m_cols.reserve(grid.width());
    for (unsigned int x = 0u; x < grid.width(); x++)
    {
        result.m_cols.emplace_back(get_constraint_from(grid.get_line<Line::COL>(x)));
    }

    assert(result.width() == grid.width());
    assert(result.height() == grid.height());
    return result;
}

} // namespace picross
