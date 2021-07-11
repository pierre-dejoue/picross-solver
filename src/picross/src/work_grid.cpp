/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#include "work_grid.h"

#include "line.h"

#include <picross/picross.h>

#include <algorithm>
#include <cassert>


namespace picross
{

constexpr unsigned int LineSelectionPolicy_RampUpMaxNbAlternatives::min_nb_alternatives;
constexpr unsigned int LineSelectionPolicy_RampUpMaxNbAlternatives::max_nb_alternatives;

namespace
{
    std::vector<LineConstraint> row_constraints_from(const InputGrid& grid)
    {
        std::vector<LineConstraint> rows;
        rows.reserve(grid.rows.size());
        std::transform(grid.rows.cbegin(), grid.rows.cend(), std::back_inserter(rows), [](const InputGrid::Constraint& c) { return LineConstraint(Line::ROW, c); });
        return rows;
    }

    std::vector<LineConstraint> column_constraints_from(const InputGrid& grid)
    {
        std::vector<LineConstraint> cols;
        cols.reserve(grid.cols.size());
        std::transform(grid.cols.cbegin(), grid.cols.cend(), std::back_inserter(cols), [](const InputGrid::Constraint& c) { return LineConstraint(Line::COL, c); });
        return cols;
    }
}  // namespace


template <typename LineSelectionPolicy>
WorkGrid<LineSelectionPolicy>::WorkGrid(const InputGrid& grid, Solver::Solutions* solutions, GridStats* stats, Solver::Observer observer, Solver::Abort abort_function)
    : OutputGrid(grid.cols.size(), grid.rows.size(), grid.name)
    , rows(row_constraints_from(grid))
    , cols(column_constraints_from(grid))
    , saved_solutions(solutions)
    , stats(stats)
    , observer(std::move(observer))
    , abort_function(std::move(abort_function))
    , max_nb_alternatives(LineSelectionPolicy::initial_max_nb_alternatives())
    , nested_level(0u)
    , binomial(new BinomialCoefficientsCache())
{
    assert(cols.size() == get_width());
    assert(rows.size() == get_height());
    assert(saved_solutions != nullptr);

    // Other initializations
    line_completed[Line::ROW].resize(get_height(), false);
    line_completed[Line::COL].resize(get_width(), false);
    line_to_be_reduced[Line::ROW].resize(get_height(), false);
    line_to_be_reduced[Line::COL].resize(get_width(), false);
    nb_alternatives[Line::ROW].resize(get_height(), 0u);
    nb_alternatives[Line::COL].resize(get_width(), 0u);
}


// Shallow copy (does not copy the list of alternatives)
template <typename LineSelectionPolicy>
WorkGrid<LineSelectionPolicy>::WorkGrid(const WorkGrid& parent, unsigned int nested_level)
    : OutputGrid(static_cast<const OutputGrid&>(parent))
    , rows(parent.rows)
    , cols(parent.cols)
    , saved_solutions(parent.saved_solutions)
    , stats(parent.stats)
    , observer(parent.observer)
    , abort_function(parent.abort_function)
    , max_nb_alternatives(LineSelectionPolicy::initial_max_nb_alternatives())
    , nested_level(nested_level)
    , binomial(nullptr)               // only used on the first pass on the grid threfore on nested_level == 0
{
    assert(nested_level > 0u);

    line_completed[Line::ROW] = parent.line_completed[Line::ROW];
    line_completed[Line::COL] = parent.line_completed[Line::COL];

    line_to_be_reduced[Line::ROW] = parent.line_to_be_reduced[Line::ROW];
    line_to_be_reduced[Line::COL] = parent.line_to_be_reduced[Line::COL];

    nb_alternatives[Line::ROW] = parent.nb_alternatives[Line::ROW];
    nb_alternatives[Line::COL] = parent.nb_alternatives[Line::COL];

    // Stats
    if (stats != nullptr && nested_level > stats->max_nested_level)
    {
        stats->max_nested_level = nested_level;
    }

    // Solver::Observer
    if (observer)
    {
        observer(Solver::Event::BRANCHING, nullptr, nested_level);
    }
}


template <typename LineSelectionPolicy>
Solver::Status WorkGrid<LineSelectionPolicy>::solve(unsigned int max_nb_solutions)
{
    auto status = full_grid_pass(nested_level == 0u);     // If nested_level == 0, this is the first pass on the grid
    if (status.contradictory)
        return Solver::Status::CONTRADICTORY_GRID;

    bool grid_completed = all_lines_completed();

    // While the reduce method is making progress, call it!
    while (!grid_completed)
    {
        status = full_grid_pass();
        if (status.contradictory)
            return Solver::Status::CONTRADICTORY_GRID;

        grid_completed = all_lines_completed();

        // Exit loop either if the grid has completed or if the condition to switch the branching search is met
        if (LineSelectionPolicy::switch_to_branching(max_nb_alternatives, status.grid_changed, status.skipped_lines, nested_level))
            break;

        // Max number of alternatives for the next full grid pass
        max_nb_alternatives = LineSelectionPolicy::get_max_nb_alternatives(max_nb_alternatives, status.grid_changed, status.skipped_lines, nested_level);
    }

    // Are we done?
    if (grid_completed)
    {
        if (observer)
        {
            observer(Solver::Event::SOLVED_GRID, nullptr, nested_level);
        }
        save_solution();
        return Solver::Status::OK;
    }
    // If we are not, we have to make an hypothesis and continue based on that
    else
    {
        // Find the row or column not yet solved with the minimal alternative lines.
        // That is the min of all alternatives greater or equal to 2.
        unsigned int min_alt = 0u;
        Line::Type found_line_type = Line::ROW;
        unsigned int found_line_index = 0u;
        for (const auto& type : { Line::ROW, Line::COL })
        {
            for (unsigned int idx = 0u; idx < nb_alternatives[type].size(); idx++)
            {
                const auto nb_alt = line_to_be_reduced[type][idx] ? 0u : nb_alternatives[type][idx];
                if (nb_alt >= 2u && (min_alt < 2u || nb_alt < min_alt))
                {
                    min_alt = nb_alt;
                    found_line_type = type;
                    found_line_index = idx;
                }
            }
        }

        if (min_alt == 0u)
        {
            Solver::Status::CONTRADICTORY_GRID;
        }

        // Select the row or column with the minimal number of alternatives
        const LineConstraint& line_constraint = found_line_type == Line::ROW ? rows.at(found_line_index) : cols.at(found_line_index);
        const Line known_tiles = get_line(found_line_type, found_line_index);

        guess_list_of_all_alternatives = line_constraint.build_all_possible_lines(known_tiles);
        assert(guess_list_of_all_alternatives.size() == min_alt);

        // Guess!
        return guess(max_nb_solutions);
    }
}


template <typename LineSelectionPolicy>
bool WorkGrid<LineSelectionPolicy>::all_lines_completed() const
{
    const bool all_rows = std::all_of(line_completed[Line::ROW].cbegin(), line_completed[Line::ROW].cend(), [](bool b) { return b; });
    const bool all_cols = std::all_of(line_completed[Line::COL].cbegin(), line_completed[Line::COL].cend(), [](bool b) { return b; });

    // The logical AND is important here: in the case an hypothesis is made on a row (resp. a column), it is marked as completed
    // but the constraints on the columns (resp. the rows) may not be all satisfied.
    return all_rows && all_cols;
}


template <typename LineSelectionPolicy>
bool WorkGrid<LineSelectionPolicy>::set_line(const Line& line)
{
    bool line_changed = false;
    const size_t line_index = line.get_index();
    const Line origin_line = get_line(line.get_type(), line_index);
    const auto width = static_cast<unsigned int>(get_width());
    const auto height = static_cast<unsigned int>(get_height());
    assert(line.size() == line.get_type() == Line::ROW ? width : height);
    const Line::Container& tiles = line.get_tiles();

    if (line.get_type() == Line::ROW)
    {
        for (unsigned int tile_index = 0u; tile_index < line.size(); tile_index++)
        {
            // modify grid
            const bool tile_changed = set(tile_index, line_index, tiles[tile_index]);

            if (tile_changed)
            {
                // mark the impacted column with flag "to be reduced"
                line_to_be_reduced[Line::COL][tile_index] = true;
                LineSelectionPolicy::estimate_nb_alternatives(nb_alternatives[Line::COL][tile_index]);

                line_changed = true;
            }
        }
    }
    else
    {
        for (unsigned int tile_index = 0u; tile_index < line.size(); tile_index++)
        {
            // modify grid
            const bool tile_changed = set(line_index, tile_index, tiles[tile_index]);

            if (tile_changed)
            {
                // mark the impacted row with flag "to be reduced"
                line_to_be_reduced[Line::ROW][tile_index] = true;
                LineSelectionPolicy::estimate_nb_alternatives(nb_alternatives[Line::ROW][tile_index]);

                line_changed = true;
            }
        }
    }
    if (observer && line_changed)
    {
        const Line delta = line_delta(origin_line, get_line(line.get_type(), line_index));
        observer(Solver::Event::DELTA_LINE, &delta, nested_level);
    }
    return line_changed;
}


// On the first pass of the grid, assume that the color of every tile in the line is unknown
// and compute the trivial reduction and number of alternatives
template <typename LineSelectionPolicy>
typename WorkGrid<LineSelectionPolicy>::PassStatus WorkGrid<LineSelectionPolicy>::single_line_initial_pass(Line::Type type, unsigned int index)
{
    PassStatus status;
    const LineConstraint& constraint = type == Line::ROW ? rows.at(index) : cols.at(index);

    const auto line_size = static_cast<unsigned int>(type == Line::ROW ? get_width() : get_height());

    assert(binomial);
    Line reduced_line(type, index, line_size);  // All Tile::UNKNOWN

    // Compute the trivial reduction if it exists and the number of alternatives
    const auto pair = constraint.line_trivial_reduction(reduced_line, *binomial);

    const auto nb_alt = pair.second;
    assert(nb_alt > 0u);

    if (stats != nullptr) { stats->max_initial_nb_alternatives = std::max(stats->max_initial_nb_alternatives, nb_alt); }

    // If the reduced line is not compatible with the information already present in the grid
    // then the row and column constraints are contradictory.
    if (!get_line(type, index).compatible(reduced_line))
    {
        status.contradictory = true;
        return status;
    }

    // Set line
    status.grid_changed = set_line(reduced_line);

    // Must be set after set_line (it modifies nb_alternatives)
    nb_alternatives[type][index] = nb_alt;

    if (nb_alt == 1u)
    {
        // Line is completed
        line_completed[type][index] = true;
        line_to_be_reduced[type][index] = false;
    }
    else
    {
        // During a normal pass line_to_be_reduced is set to false after a line reduction has been performed.
        // Here since we are computing a trivial reduction assuming the initial line is completly unknown we
        // are ignoring tiles that are possibly already set. In such a case, we need to redo a reduction
        // on the next full grid pass.
        const Line new_line = get_line(type, index);
        if (reduced_line == new_line)
        {
            line_completed[type][index] = false;
            line_to_be_reduced[type][index] = false;
        }
        else
        {
            line_completed[type][index] = is_fully_defined(new_line);
            line_to_be_reduced[type][index] = !line_completed[type][index];
            if (line_completed[type][index]) { nb_alternatives[type][index] = 1u;  }
        }
    }

    return status;
}


template <typename LineSelectionPolicy>
typename WorkGrid<LineSelectionPolicy>::PassStatus WorkGrid<LineSelectionPolicy>::single_line_pass(Line::Type type, unsigned int index)
{
    assert(line_completed[type][index] == false);
    assert(line_to_be_reduced[type][index] == true);

    if (stats != nullptr) { stats->nb_single_line_pass_calls++; }

    PassStatus status;
    const LineConstraint& line_constraint = type == Line::ROW ? rows.at(index) : cols.at(index);
    const Line known_tiles = get_line(type, index);

    // Reduce all possible lines that match the data already present in the grid and the line constraint
    const auto reduction = line_constraint.reduce_and_count_alternatives(known_tiles, stats);
    const Line& reduced_line = reduction.first;
    const auto nb_alt = reduction.second;

    if (stats != nullptr) { stats->max_nb_alternatives = std::max(stats->max_nb_alternatives, nb_alt); }

    // If the list of all lines is empty, it means the grid data is contradictory. Throw an exception.
    if (nb_alt == 0)
    {
        status.contradictory = true;
        return status;
    }

    // In any case, update the grid data with the reduced line resulting from list all_lines
    status.grid_changed = set_line(reduced_line);

    // Must be set after set_line (it modifies nb_alternatives)
    nb_alternatives[type][index] = nb_alt;

    // If the list comprises of only one element, it means we solved that line
    if (nb_alt == 1) { line_completed[type][index] = true; }


    if (stats != nullptr && status.grid_changed)
    {
        stats->nb_single_line_pass_calls_w_change++;
        stats->max_nb_alternatives_w_change = std::max(stats->max_nb_alternatives_w_change, nb_alt);
    }

    // This line does not need to be reduced until one of the tiles is modified.
    line_to_be_reduced[type][index] = false;

    return status;
}


// Reduce all rows or all columns. Return false if no change was made on the grid.
template <typename LineSelectionPolicy>
typename WorkGrid<LineSelectionPolicy>::PassStatus WorkGrid<LineSelectionPolicy>::full_side_pass(Line::Type type, bool first_pass)
{
    PassStatus status;
    const auto length = type == Line::ROW ? get_height() : get_width();

    if (first_pass)
    {
        for (unsigned int x = 0u; x < length; x++)
        {
            status += single_line_initial_pass(type, x);
            if (status.contradictory)
                break;
        }
    }
    else
    {
        for (unsigned int x = 0u; x < length; x++)
        {
            if (line_to_be_reduced[type][x])
            {
                if (nb_alternatives[type][x] <= max_nb_alternatives)
                {
                    status += single_line_pass(type, x);
                    if (status.contradictory)
                        break;
                }
                else
                {
                    status.skipped_lines++;
                }
            }
            if (abort_function && abort_function()) { throw PicrossSolverAborted(); }
        }
    }
    return status;
}


// Reduce all columns and all rows. Return false if no change was made on the grid.
// Return true if the grid was changed during the full pass
template <typename LineSelectionPolicy>
typename WorkGrid<LineSelectionPolicy>::PassStatus WorkGrid<LineSelectionPolicy>::full_grid_pass(bool first_pass)
{
    PassStatus status;
    if (stats != nullptr) { stats->nb_full_grid_pass_calls++; }

    // Pass on columns
    status += full_side_pass(Line::COL, first_pass);
    if (status.contradictory)
        return status;

    // Pass on rows
    status += full_side_pass(Line::ROW, first_pass);
    return status;
}


template <typename LineSelectionPolicy>
Solver::Status WorkGrid<LineSelectionPolicy>::guess(unsigned int max_nb_solutions) const
{
    assert(guess_list_of_all_alternatives.size() > 0u);
    /* This function will test a range of alternatives for one particular line of the grid, each time
     * creating a new instance of the grid class on which the function WorkGrid::solve() is called.
     */

    if (stats != nullptr)
    {
        stats->guess_total_calls++;
        const auto nb_alternatives = static_cast<unsigned int>(guess_list_of_all_alternatives.size());
        stats->guess_total_alternatives += nb_alternatives;
        if (stats->guess_max_nb_alternatives_by_depth.size() < nested_level + 1)
            stats->guess_max_nb_alternatives_by_depth.resize(nested_level + 1);
        auto& max_nb_alternatives = stats->guess_max_nb_alternatives_by_depth[nested_level];
        max_nb_alternatives = std::max(max_nb_alternatives, nb_alternatives);
    }

    const Line::Type guess_line_type = guess_list_of_all_alternatives.front().get_type();
    const unsigned int guess_line_index = guess_list_of_all_alternatives.front().get_index();
    bool flag_solution_found = false;
    for (const Line& guess_line : guess_list_of_all_alternatives)
    {
        // Allocate a new work grid. Use the shallow copy.
        WorkGrid new_grid(*this, nested_level + 1);

        // Set one line in the new_grid according to the hypothesis we made. That line is then complete
        const bool changed = new_grid.set_line(guess_line);
        assert(changed);
        assert(!new_grid.line_completed[guess_line_type][guess_line_index]);
        new_grid.line_completed[guess_line_type][guess_line_index] = true;
        new_grid.line_to_be_reduced[guess_line_type][guess_line_index] = false;
        new_grid.nb_alternatives[guess_line_type][guess_line_index] = 1u;

        if (max_nb_solutions > 0u && saved_solutions->size() >= max_nb_solutions)
            break;

        // Solve the new grid!
        const auto status = new_grid.solve(max_nb_solutions);
        flag_solution_found |= status == Solver::Status::OK;
    }
    return flag_solution_found ? Solver::Status::OK : Solver::Status::CONTRADICTORY_GRID;
}


template <typename LineSelectionPolicy>
bool WorkGrid<LineSelectionPolicy>::valid_solution() const
{
    assert(is_solved());
    bool valid = true;
    for (unsigned int x = 0u; x < get_width(); x++) { valid &= cols.at(x).compatible(get_line<Line::COL>(x)); }
    for (unsigned int y = 0u; y < get_height(); y++) { valid &= rows.at(y).compatible(get_line<Line::ROW>(y)); }
    return valid;
}


template <typename LineSelectionPolicy>
void WorkGrid<LineSelectionPolicy>::save_solution() const
{
    assert(valid_solution());
    if (stats != nullptr) { stats->nb_solutions++; }

    // Shallow copy of only the grid data
    saved_solutions->emplace_back(static_cast<const OutputGrid&>(*this));
}


// Template explicit instantiations
template class WorkGrid<LineSelectionPolicy_Legacy>;
template class WorkGrid<LineSelectionPolicy_RampUpMaxNbAlternatives>;
template class WorkGrid<LineSelectionPolicy_RampUpMaxNbAlternatives_EstimateNbAlternatives>;


} // namespace picross
