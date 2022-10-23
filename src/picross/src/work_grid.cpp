/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2022 Pierre DEJOUE
 ******************************************************************************/
#include "work_grid.h"

#include "line.h"

#include <picross/picross.h>

#include <algorithm>
#include <cassert>
#include <memory>


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

void merge_nested_grid_stats(GridStats& stats, const GridStats& nested_stats)
{
    stats.nb_solutions += nested_stats.nb_solutions;
    // stats.max_nb_solutions
    stats.max_branching_depth = std::max(stats.max_branching_depth, nested_stats.max_branching_depth);
    stats.nb_branching_calls += nested_stats.nb_branching_calls;
    stats.total_nb_branching_alternatives += nested_stats.total_nb_branching_alternatives;

    stats.max_nb_alternatives_by_branching_depth.resize(stats.max_branching_depth, 0u);
    for (size_t d = 0; d < nested_stats.max_nb_alternatives_by_branching_depth.size(); d++)
    {
        assert(d < stats.max_nb_alternatives_by_branching_depth.size());
        stats.max_nb_alternatives_by_branching_depth[d] = std::max(stats.max_nb_alternatives_by_branching_depth[d], nested_stats.max_nb_alternatives_by_branching_depth[d]);
    }

    stats.max_initial_nb_alternatives = std::max(stats.max_initial_nb_alternatives, nested_stats.max_initial_nb_alternatives);
    stats.max_nb_alternatives = std::max(stats.max_nb_alternatives, nested_stats.max_nb_alternatives);
    stats.max_nb_alternatives_w_change = std::max(stats.max_nb_alternatives_w_change, nested_stats.max_nb_alternatives_w_change);
    stats.nb_reduce_list_of_lines_calls += nested_stats.nb_reduce_list_of_lines_calls;
    stats.max_reduce_list_size = std::max(stats.max_reduce_list_size, nested_stats.max_reduce_list_size);
    stats.total_lines_reduced += nested_stats.total_lines_reduced;
    stats.nb_reduce_and_count_alternatives_calls += nested_stats.nb_reduce_and_count_alternatives_calls;
    stats.nb_full_grid_pass_calls += nested_stats.nb_full_grid_pass_calls;
    stats.nb_single_line_pass_calls += nested_stats.nb_single_line_pass_calls;
    stats.nb_single_line_pass_calls_w_change += nested_stats.nb_single_line_pass_calls_w_change;
    stats.nb_observer_callback_calls += nested_stats.nb_observer_callback_calls;
}

}  // namespace


template <typename LineSelectionPolicy, bool BranchingAllowed>
WorkGrid<LineSelectionPolicy, BranchingAllowed>::WorkGrid(const InputGrid& grid, Solver::Observer observer, Solver::Abort abort_function)
    : OutputGrid(grid.cols.size(), grid.rows.size(), grid.name)
    , rows(row_constraints_from(grid))
    , cols(column_constraints_from(grid))
    , stats(nullptr)
    , observer(std::move(observer))
    , abort_function(std::move(abort_function))
    , max_nb_alternatives(LineSelectionPolicy::initial_max_nb_alternatives())
    , branching_depth(0u)
    , binomial(new BinomialCoefficientsCache())
{
    assert(cols.size() == width());
    assert(rows.size() == height());

    // Other initializations
    line_completed[Line::ROW].resize(height(), false);
    line_completed[Line::COL].resize(width(), false);
    line_to_be_reduced[Line::ROW].resize(height(), false);
    line_to_be_reduced[Line::COL].resize(width(), false);
    nb_alternatives[Line::ROW].resize(height(), 0u);
    nb_alternatives[Line::COL].resize(width(), 0u);
}


// Shallow copy (does not copy the list of alternatives)
template <typename LineSelectionPolicy, bool BranchingAllowed>
WorkGrid<LineSelectionPolicy, BranchingAllowed>::WorkGrid(const WorkGrid& parent, unsigned int nested_level)
    : OutputGrid(static_cast<const OutputGrid&>(parent))
    , rows(parent.rows)
    , cols(parent.cols)
    , stats(nullptr)
    , observer(parent.observer)
    , abort_function(parent.abort_function)
    , max_nb_alternatives(LineSelectionPolicy::initial_max_nb_alternatives())
    , branching_depth(nested_level)
    , binomial(nullptr)               // only used on the first pass on the grid threfore on nested_level == 0
{
    assert(nested_level > 0u);

    line_completed[Line::ROW] = parent.line_completed[Line::ROW];
    line_completed[Line::COL] = parent.line_completed[Line::COL];

    line_to_be_reduced[Line::ROW] = parent.line_to_be_reduced[Line::ROW];
    line_to_be_reduced[Line::COL] = parent.line_to_be_reduced[Line::COL];

    nb_alternatives[Line::ROW] = parent.nb_alternatives[Line::ROW];
    nb_alternatives[Line::COL] = parent.nb_alternatives[Line::COL];

    // Solver::Observer
    if (observer)
    {
        observer(Solver::Event::BRANCHING, nullptr, nested_level);
    }
}

template <typename LineSelectionPolicy, bool BranchingAllowed>
void WorkGrid<LineSelectionPolicy, BranchingAllowed>::set_stats(GridStats* stats)
{
    this->stats = stats;
    if (stats)
        stats->max_branching_depth = branching_depth;
}


template <typename LineSelectionPolicy, bool BranchingAllowed>
Solver::Status WorkGrid<LineSelectionPolicy, BranchingAllowed>::line_solve(Solver::Solutions& solutions)
{
    auto pass_status = full_grid_pass(branching_depth == 0u);     // If nested_level == 0, this is the first pass on the grid
    if (pass_status.contradictory)
        return Solver::Status::CONTRADICTORY_GRID;

    bool grid_completed = all_lines_completed();

    // While the reduce method is making progress, call it!
    while (!grid_completed)
    {
        pass_status = full_grid_pass();
        if (pass_status.contradictory)
            return Solver::Status::CONTRADICTORY_GRID;

        grid_completed = all_lines_completed();

        // Exit loop either if the grid has completed or if the condition to switch the branching search is met
        if (LineSelectionPolicy::switch_to_branching(max_nb_alternatives, pass_status.grid_changed, pass_status.skipped_lines, branching_depth))
            break;

        // Max number of alternatives for the next full grid pass
        max_nb_alternatives = LineSelectionPolicy::get_max_nb_alternatives(max_nb_alternatives, pass_status.grid_changed, pass_status.skipped_lines, branching_depth);
    }

    // Are we done?
    if (grid_completed)
    {
        if (observer)
        {
            observer(Solver::Event::SOLVED_GRID, nullptr, branching_depth);
        }
        save_solution(solutions);
        return Solver::Status::OK;
    }
    // If we are not, the grid is not line solvable
    else
    {
        return Solver::Status::NOT_LINE_SOLVABLE;
    }
}


template <typename LineSelectionPolicy, bool BranchingAllowed>
Solver::Status WorkGrid<LineSelectionPolicy, BranchingAllowed>::solve(Solver::Solutions& solutions, unsigned int max_nb_solutions)
{
    auto status = line_solve(solutions);

    if (status == Solver::Status::NOT_LINE_SOLVABLE)
    {
        if constexpr (BranchingAllowed)
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

            if (min_alt > 0u)
            {
                // Select the row or column with the minimal number of alternatives
                const LineConstraint& line_constraint = found_line_type == Line::ROW ? rows.at(found_line_index) : cols.at(found_line_index);
                const Line known_tiles = get_line(found_line_type, found_line_index);

                guess_list_of_all_alternatives = line_constraint.build_all_possible_lines(known_tiles);
                assert(guess_list_of_all_alternatives.size() == min_alt);

                // Make a guess
                status = branch(solutions, max_nb_solutions);
            }
            else
            {
                status = Solver::Status::CONTRADICTORY_GRID;
            }
        }
        else  // No branching allowed
        {
            // Store the incomplete solution
            save_solution(solutions);
        }
    }

    return status;
}


template <typename LineSelectionPolicy, bool BranchingAllowed>
bool WorkGrid<LineSelectionPolicy, BranchingAllowed>::all_lines_completed() const
{
    const bool all_rows = std::all_of(std::cbegin(line_completed[Line::ROW]), std::cend(line_completed[Line::ROW]), [](bool b) { return b; });
    const bool all_cols = std::all_of(std::cbegin(line_completed[Line::COL]), std::cend(line_completed[Line::COL]), [](bool b) { return b; });

    // The logical AND is important here: in the case an hypothesis is made on a row (resp. a column), it is marked as completed
    // but the constraints on the columns (resp. the rows) may not be all satisfied.
    return all_rows & all_cols;
}


template <typename LineSelectionPolicy, bool BranchingAllowed>
bool WorkGrid<LineSelectionPolicy, BranchingAllowed>::set_line(const Line& line, unsigned int nb_alt)
{
    const auto line_type = line.type();
    const size_t line_index = line.index();
    const Line observer_original_line = observer ? get_line(line.type(), line_index) : Line(Line::ROW, 0, 0);
    assert(line.size() == static_cast<unsigned int>(line.type() == Line::ROW ? width() : height()));
    const Line::Container& tiles = line.tiles();

    bool line_changed = false;
    const auto set_tile_func = [this, &line_changed](Line::Type type, unsigned int idx) {
        // mark the impacted line or column with flag "to be reduced"
        line_to_be_reduced[type][idx] = true;
        LineSelectionPolicy::estimate_nb_alternatives(nb_alternatives[type][idx]);
        line_changed = true;
    };

    bool line_is_complete = true;
    if (line_type == Line::ROW)
    {
        for (unsigned int tile_index = 0u; tile_index < line.size(); tile_index++)
        {
            const bool tile_changed = set(tile_index, line_index, tiles[tile_index]);
            line_is_complete &= (get(tile_index, line_index) != Tile::UNKNOWN);
            if (tile_changed)
                set_tile_func(Line::COL, tile_index);
        }
    }
    else
    {
        for (unsigned int tile_index = 0u; tile_index < line.size(); tile_index++)
        {
            const bool tile_changed = set(line_index, tile_index, tiles[tile_index]);
            line_is_complete &= (get(line_index, tile_index) != Tile::UNKNOWN);
            if (tile_changed)
                set_tile_func(Line::ROW, tile_index);
        }
    }

    if (nb_alt > 0) { nb_alternatives[line_type][line_index] = nb_alt; }

    line_completed[line_type][line_index] = line_is_complete;

    // Most of the time after a line is set, it does not need to be reduced another time
    // Exceptions to the rule must be handled by the caller
    line_to_be_reduced[line_type][line_index] = false;

    if (observer && line_changed)
    {
        const Line delta = line_delta(observer_original_line, get_line(line_type, line_index));
        observer(Solver::Event::DELTA_LINE, &delta, branching_depth);
    }
    return line_changed;
}


// On the first pass of the grid, assume that the color of every tile in the line is unknown
// and compute the trivial reduction and number of alternatives
template <typename LineSelectionPolicy, bool BranchingAllowed>
typename WorkGrid<LineSelectionPolicy, BranchingAllowed>::PassStatus WorkGrid<LineSelectionPolicy, BranchingAllowed>::single_line_initial_pass(Line::Type type, unsigned int index)
{
    PassStatus status;
    const LineConstraint& constraint = type == Line::ROW ? rows.at(index) : cols.at(index);

    const auto line_size = static_cast<unsigned int>(type == Line::ROW ? width() : height());

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
    status.grid_changed = set_line(reduced_line, nb_alt);

    if (nb_alt > 1u)
    {
        // During a normal pass line_to_be_reduced is set to false after a line reduction has been performed.
        // Here since we are computing a trivial reduction assuming the initial line is completly unknown we
        // are ignoring tiles that are possibly already set. In such a case, we need to redo a reduction
        // on the next full grid pass.
        const Line new_line = get_line(type, index);
        if (reduced_line != new_line)
        {
            line_to_be_reduced[type][index] = !line_completed[type][index];
        }
    }

    return status;
}


template <typename LineSelectionPolicy, bool BranchingAllowed>
typename WorkGrid<LineSelectionPolicy, BranchingAllowed>::PassStatus WorkGrid<LineSelectionPolicy, BranchingAllowed>::single_line_pass(Line::Type type, unsigned int index)
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
    status.grid_changed = set_line(reduced_line, nb_alt);

    if (stats != nullptr && status.grid_changed)
    {
        stats->nb_single_line_pass_calls_w_change++;
        stats->max_nb_alternatives_w_change = std::max(stats->max_nb_alternatives_w_change, nb_alt);
    }

    return status;
}


// Reduce all rows or all columns. Return false if no change was made on the grid.
template <typename LineSelectionPolicy, bool BranchingAllowed>
typename WorkGrid<LineSelectionPolicy, BranchingAllowed>::PassStatus WorkGrid<LineSelectionPolicy, BranchingAllowed>::full_side_pass(Line::Type type, bool first_pass)
{
    PassStatus status;
    const auto length = type == Line::ROW ? height() : width();

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
            if (abort_function && abort_function())
                throw PicrossSolverAborted();
        }
    }
    return status;
}


// Reduce all columns and all rows. Return false if no change was made on the grid.
// Return true if the grid was changed during the full pass
template <typename LineSelectionPolicy, bool BranchingAllowed>
typename WorkGrid<LineSelectionPolicy, BranchingAllowed>::PassStatus WorkGrid<LineSelectionPolicy, BranchingAllowed>::full_grid_pass(bool first_pass)
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


template <typename LineSelectionPolicy, bool BranchingAllowed>
Solver::Status WorkGrid<LineSelectionPolicy, BranchingAllowed>::branch(Solver::Solutions& solutions, unsigned int max_nb_solutions) const
{
    assert(guess_list_of_all_alternatives.size() > 0u);
    /* This function will test a range of alternatives for one particular line of the grid, each time
     * creating a new instance of the grid class on which the function WorkGrid<LineSelectionPolicy, BranchingAllowed>::solve() is called.
     */

    if (stats != nullptr)
    {
        stats->nb_branching_calls++;
        const auto nb_alternatives = static_cast<unsigned int>(guess_list_of_all_alternatives.size());
        stats->total_nb_branching_alternatives += nb_alternatives;
        if (stats->max_nb_alternatives_by_branching_depth.size() < branching_depth + 1)
            stats->max_nb_alternatives_by_branching_depth.resize(branching_depth + 1, 0u);
        auto& max_nb_alternatives = stats->max_nb_alternatives_by_branching_depth[branching_depth];
        max_nb_alternatives = std::max(max_nb_alternatives, nb_alternatives);
    }

    const Line::Type guess_line_type = guess_list_of_all_alternatives.front().type();
    const unsigned int guess_line_index = guess_list_of_all_alternatives.front().index();
    bool flag_solution_found = false;
    for (const Line& guess_line : guess_list_of_all_alternatives)
    {
        // Allocate a new work grid. Use the shallow copy.
        WorkGrid new_grid(*this, branching_depth + 1);
        Solver::Solutions nested_solutions;
        std::unique_ptr<GridStats> nested_stats;
        if (stats)
            nested_stats = std::make_unique<GridStats>();

        // Set one line in the new_grid according to the hypothesis we made. That line is then complete
        const bool changed = new_grid.set_line(guess_line, 1u);
        assert(changed);

        // Solve the new grid!
        new_grid.set_stats(nested_stats.get());
        const auto status = new_grid.solve(nested_solutions, max_nb_solutions);

        if (stats)
        {
            assert(nested_stats);
            merge_nested_grid_stats(*stats, *nested_stats);
        }

        flag_solution_found |= status == Solver::Status::OK;

        solutions.insert(solutions.end(), nested_solutions.begin(), nested_solutions.end());

        // Stop if enough solutions found
        if (max_nb_solutions > 0u && solutions.size() >= max_nb_solutions)
            break;
    }
    return flag_solution_found ? Solver::Status::OK : Solver::Status::CONTRADICTORY_GRID;
}


template <typename LineSelectionPolicy, bool BranchingAllowed>
bool WorkGrid<LineSelectionPolicy, BranchingAllowed>::valid_solution() const
{
    assert(is_solved());
    bool valid = true;
    for (unsigned int x = 0u; x < width(); x++) { valid &= cols.at(x).compatible(get_line<Line::COL>(x)); }
    for (unsigned int y = 0u; y < height(); y++) { valid &= rows.at(y).compatible(get_line<Line::ROW>(y)); }
    return valid;
}


template <typename LineSelectionPolicy, bool BranchingAllowed>
void WorkGrid<LineSelectionPolicy, BranchingAllowed>::save_solution(Solver::Solutions& solutions) const
{
    if constexpr (BranchingAllowed)
    {
        assert(valid_solution());
        if (stats != nullptr) { stats->nb_solutions++; }
    }
    else
    {
        assert(branching_depth == 0);
    }

    // Shallow copy of only the grid data
    solutions.emplace_back(Solver::Solution{ static_cast<const OutputGrid&>(*this), branching_depth });
}


// Template explicit instantiations
template class WorkGrid<LineSelectionPolicy_Legacy, true>;
template class WorkGrid<LineSelectionPolicy_RampUpMaxNbAlternatives, true>;
template class WorkGrid<LineSelectionPolicy_RampUpMaxNbAlternatives_EstimateNbAlternatives, true>;
template class WorkGrid<LineSelectionPolicy_RampUpMaxNbAlternatives_EstimateNbAlternatives, false>;

} // namespace picross
