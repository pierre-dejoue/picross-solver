/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#include "line_constraint.h"

#include "line.h"

#include <algorithm>
#include <cassert>
#include <numeric>


namespace picross
{

LineConstraint::LineConstraint(Line::Type type, const InputGrid::Constraint& vect) :
    type(type),
    segs_of_ones(vect)
{
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
    // Sets_of_ones vector must not be empty.
    if (segs_of_ones.empty()) { return 0u; }

    // Compute max
    return *max_element(segs_of_ones.cbegin(), segs_of_ones.cend());
}


/*
 * Given an uninitialized line compute the trivial reduction and the theoritical number of alternatives
 *
 *  - Set the line pass as argument to the trivial reduction, if there is one
 *  - Return a pair (changed, nb_alternatives)
 */
std::pair<bool, unsigned int> LineConstraint::line_trivial_reduction(Line& line, BinomialCoefficientsCache& binomial_cache) const
{
    assert(line.get_type() == type);
    assert(is_all_one_color(line, Tile::UNKNOWN));

    if (line.size() < min_line_size)
    {
        // This can happen if the user does not check the grid with check_grid_input()
        throw std::logic_error("Constraint::line_trivial_reduction: line.size() < min_line_size");
    }

    bool changed = false;
    unsigned int nb_alternatives = 0u;

    const unsigned int nb_zeros = line.size() - min_line_size;
    Line::Container& tiles = line.get_tiles();
    unsigned int line_idx = 0u;

    if (max_segment_size() == 0u)
    {
        // Blank line
        for (unsigned int c = 0u; c < line.size(); c++) { tiles[line_idx++] = Tile::ZERO; }

        assert(line_idx == line.size());
        assert(is_fully_defined(line));
        changed = true;
        nb_alternatives = 1;
    }
    else if (nb_zeros == 0u)
    {
        // The line is fully defined
        for (unsigned int seg_idx = 0u; seg_idx < segs_of_ones.size(); seg_idx++)
        {
            const auto seg_sz = segs_of_ones[seg_idx];
            const bool last = (seg_idx + 1u == segs_of_ones.size());
            for (unsigned int c = 0u; c < seg_sz; c++) { tiles[line_idx++] = Tile::ONE; }
            if (!last) { tiles[line_idx++] = Tile::ZERO; }
        }

        assert(line_idx == line.size());
        assert(is_fully_defined(line));
        changed = true;
        nb_alternatives = 1;
    }
    else
    {
        if (max_segment_size() > nb_zeros)
        {
            // The reduction is straightforward in that case
            for (unsigned int seg_idx = 0u; seg_idx < segs_of_ones.size(); seg_idx++)
            {
                const auto seg_sz = segs_of_ones[seg_idx];
                const bool last = (seg_idx + 1u == segs_of_ones.size());
                for (unsigned int c = 0u; c < seg_sz; c++) { if (c >= nb_zeros) { tiles[line_idx] = Tile::ONE; }; line_idx++; }
                if (!last) { line_idx++; /* would be Tile::ZERO */ }
            }

            assert(line_idx + nb_zeros == line.size());
            assert(!is_all_one_color(line, Tile::UNKNOWN));
            changed = true;
        }
        nb_alternatives = binomial_cache.nb_alternatives_for_fixed_nb_of_partitions(nb_zeros, static_cast<unsigned int>(segs_of_ones.size()) + 1u);
    }

    return std::make_pair(changed, nb_alternatives);
}


std::vector<Line> LineConstraint::build_all_possible_lines(const Line& known_tiles) const
{
    if (known_tiles.get_type() != type) { throw std::invalid_argument("Constraint::build_all_possible_lines: Wrong filter line type"); }
    const size_t index = known_tiles.get_index();

    // Number of zeros to add to the minimal size line.
    if (known_tiles.size() < min_line_size) { throw std::logic_error("Constraint::build_all_possible_lines: line_size < min_line_size"); }
    unsigned int nb_zeros = known_tiles.size() - min_line_size;

    std::vector<Line> result;
    Line new_line(known_tiles, Tile::UNKNOWN);
    Line::Container& new_tile_vect = new_line.get_tiles();

    if (segs_of_ones.size() == 0)
    {
        // Return a list with only one all-zero line
        unsigned int line_idx = 0u;
        for (unsigned int c = 0u; c < known_tiles.size(); c++) { new_tile_vect[line_idx++] = Tile::ZERO; }

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
            for (unsigned int c = 0u; c < n; c++) { new_tile_vect[line_idx++] = Tile::ZERO; }
            for (unsigned int c = 0u; c < segs_of_ones[0]; c++) { new_tile_vect[line_idx++] = Tile::ONE; }
            while (line_idx < known_tiles.size()) { new_tile_vect[line_idx++] = Tile::ZERO; }

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
            for (unsigned int c = 0u; c < n; c++) { new_tile_vect[line_idx++] = Tile::ZERO; }
            for (unsigned int c = 0u; c < segs_of_ones[0]; c++) { new_tile_vect[line_idx++] = Tile::ONE; }
            new_tile_vect[line_idx++] = Tile::ZERO;

            // Filter against known_tiles
            if (new_line.compatible(known_tiles))
            {
                // If OK, then go on and recursively call this function to construct the remaining part of the line.
                std::vector<unsigned int> trim_sets_of_ones(segs_of_ones.begin() + 1, segs_of_ones.end());
                LineConstraint recursive_constraint(type, trim_sets_of_ones);

                std::vector<Tile::Type> end_known_vect(known_tiles.get_tiles().cbegin() + line_idx, known_tiles.get_tiles().cend());
                Line end_known_tiles(type, index, std::move(end_known_vect));

                std::vector<Line> recursive_list = recursive_constraint.build_all_possible_lines(end_known_tiles);

                // Finally, construct the return_list based on the contents of the recursive_list.
                for (const Line& line : recursive_list)
                {
                    std::copy(line.get_tiles().cbegin(), line.get_tiles().cend(), new_tile_vect.begin() + line_idx);
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


namespace
{
    // Helper class to recursively build all the possible alternatives of a Line, given a constraint
    class BuildLineAlternatives
    {
    public:
        BuildLineAlternatives(const InputGrid::Constraint& segs_of_ones, const Line& known_tiles)
            : segs_of_ones(segs_of_ones)
            , known_tiles(known_tiles)
            , reduced_line()
        {
        }

        unsigned int build_alternatives(const Line& alternative, unsigned int remaining_zeros, size_t line_idx = 0u, size_t constraint_idx = 0u)
        {
            assert(alternative.get_type() == known_tiles.get_type());
            assert(alternative.size() == known_tiles.size());

            unsigned int nb_alternatives = 0u;

            // If the last segment of ones was reached, pad end of line with zero, chack compatibility then reduce
            if (constraint_idx == segs_of_ones.size())
            {
                Line next_alternative(alternative);
                Line::Container& next_tiles = next_alternative.get_tiles();
                assert(next_tiles.size() - line_idx == remaining_zeros);
                for (unsigned int c = 0u; c < remaining_zeros; c++) { next_tiles[line_idx++] = Tile::ZERO; }

                nb_alternatives += filter_and_reduce(next_alternative);
            }
            // Else, fill in the next segment of ones, then call recursively
            else
            {
                const auto& nb_ones = segs_of_ones[constraint_idx];
                for (unsigned int pre_zeros = 0u; pre_zeros <= remaining_zeros; pre_zeros++)
                {
                    Line next_alternative(alternative);
                    auto next_line_idx = line_idx;
                    Line::Container& next_tiles = next_alternative.get_tiles();
                    for (unsigned int c = 0u; c < pre_zeros; c++) { next_tiles[next_line_idx++] = Tile::ZERO; }
                    for (unsigned int c = 0u; c < nb_ones; c++) { next_tiles[next_line_idx++] = Tile::ONE; }
                    if (constraint_idx + 1 < segs_of_ones.size()) { next_tiles[next_line_idx++] = Tile::ZERO; }

                    if (next_alternative.compatible(known_tiles))
                    {
                        nb_alternatives += build_alternatives(next_alternative, remaining_zeros - pre_zeros, next_line_idx, constraint_idx + 1);
                    }
                }
            }

            return nb_alternatives;
        };

        Line get_reduced_line()
        {
            if (reduced_line)
                return *reduced_line;
            else
                return Line(known_tiles, Tile::UNKNOWN);
        }

    private:
        unsigned int filter_and_reduce(const Line& alternative)
        {
            assert(is_fully_defined(alternative));
            if (alternative.compatible(known_tiles))
            {
                if (!reduced_line)
                {
                    reduced_line = std::make_unique<Line>(alternative);
                }
                else
                {
                    reduced_line->reduce(alternative);
                }
                return 1u;
            }
            return 0u;
        }

    private:
        const InputGrid::Constraint& segs_of_ones;
        const Line& known_tiles;
        std::unique_ptr<Line> reduced_line;
    };

}  // namespace


std::pair<Line, unsigned int> LineConstraint::reduce_and_count_alternatives(const Line& known_tiles, GridStats * stats) const
{
    if (stats != nullptr) { stats->nb_reduce_and_count_alternatives_calls++; }

    if (known_tiles.get_type() != type) { throw std::invalid_argument("Constraint::reduce_and_count_alternatives: Wrong filter line type"); }
    const size_t index = known_tiles.get_index();

    // Number of zeros to add to the minimal size line.
    if (known_tiles.size() < min_line_size) { throw std::logic_error("Constraint::reduce_and_count_alternatives: line_size < min_line_size"); }
    unsigned int nb_zeros = known_tiles.size() - min_line_size;

    BuildLineAlternatives builder(segs_of_ones, known_tiles);
    Line seed_alternative(known_tiles, Tile::UNKNOWN);
    unsigned int nb_alternatives = builder.build_alternatives(seed_alternative, nb_zeros);

    return std::make_pair(builder.get_reduced_line(), nb_alternatives);
}

bool LineConstraint::compatible(const Line& line) const
{
    assert(is_fully_defined(line));
    const auto& tiles = line.get_tiles();
    InputGrid::Constraint line_segs_of_ones;
    unsigned int idx = 0u;
    while (idx < tiles.size())
    {
        while (idx < tiles.size() && tiles[idx] == Tile::ZERO) { idx++; }
        unsigned int seg_sz = 0u;
        while (idx < tiles.size() && tiles[idx] == Tile::ONE) { idx++; seg_sz++; }
        if (seg_sz > 0)
        {
            line_segs_of_ones.push_back(seg_sz);
        }
    }
    return segs_of_ones == line_segs_of_ones;
}


void LineConstraint::print(std::ostream& ostream) const
{
    ostream << "Constraint on a " << str_line_type(type) << ": [ ";
    for (auto& it = segs_of_ones.begin(); it != segs_of_ones.end(); ++it)
    {
        ostream << *it << " ";
    }
    ostream << "]; min_line_size = " << min_line_size;
}


std::ostream& operator<<(std::ostream& ostream, const LineConstraint& constraint)
{
    constraint.print(ostream);
    return ostream;
}

} // namespace picross
