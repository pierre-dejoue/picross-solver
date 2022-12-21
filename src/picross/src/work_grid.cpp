/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2022 Pierre DEJOUE
 ******************************************************************************/
#include "work_grid.h"

#include "line.h"
#include "macros.h"

#include <picross/picross.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <tuple>


namespace picross
{

namespace
{
std::vector<LineConstraint> build_constraints_from(Line::Type type, const InputGrid& grid)
{
    std::vector<LineConstraint> output;
    const InputGrid::Constraints& input = type == Line::ROW ? grid.m_rows : grid.m_cols;
    output.reserve(input.size());
    std::transform(input.cbegin(), input.cend(), std::back_inserter(output), [type](const auto& c) { return LineConstraint(type, c); });
    return output;
}

std::vector<LineAlternatives> build_line_alternatives_from(Line::Type type, const std::vector<LineConstraint>& constraints, const Grid& grid, BinomialCoefficientsCache& binomial)
{
    std::vector<LineAlternatives> output;
    output.reserve(constraints.size());
    std::size_t idx = 0;
    std::transform(constraints.cbegin(), constraints.cend(), std::back_inserter(output), [type, &grid, &idx, &binomial](const auto& c) { return LineAlternatives(c, grid.get_line(type, idx++), binomial); });
    return output;
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
    : Grid(grid.width(), grid.height(), grid.name())
    , m_constraints()
    , m_alternatives()
    , m_line_completed()
    , m_line_to_be_reduced()
    , m_nb_alternatives()
    , m_grid_stats(nullptr)
    , m_observer(std::move(observer))
    , m_abort_function(std::move(abort_function))
    , m_max_nb_alternatives(LineSelectionPolicy::initial_max_nb_alternatives())
    , m_guess_list_of_all_alternatives()
    , m_branching_depth(0u)
    , m_binomial(std::make_shared<BinomialCoefficientsCache>())
{
    assert(m_binomial);

    m_constraints[Line::ROW] = build_constraints_from(Line::ROW, grid);
    m_constraints[Line::COL] = build_constraints_from(Line::COL, grid);
    m_alternatives[Line::ROW] = build_line_alternatives_from(Line::ROW, m_constraints[Line::ROW], static_cast<const Grid&>(*this), *m_binomial);
    m_alternatives[Line::COL] = build_line_alternatives_from(Line::COL, m_constraints[Line::COL], static_cast<const Grid&>(*this), *m_binomial);
    m_line_completed[Line::ROW].resize(height(), false);
    m_line_completed[Line::COL].resize(width(), false);
    m_line_to_be_reduced[Line::ROW].resize(height(), false);
    m_line_to_be_reduced[Line::COL].resize(width(), false);
    m_nb_alternatives[Line::ROW].resize(height(), 0u);
    m_nb_alternatives[Line::COL].resize(width(), 0u);

    assert(m_constraints[Line::ROW].size() == height());
    assert(m_constraints[Line::COL].size() == width());
}


// Shallow copy (does not copy the list of alternatives)
template <typename LineSelectionPolicy, bool BranchingAllowed>
WorkGrid<LineSelectionPolicy, BranchingAllowed>::WorkGrid(const WorkGrid& parent, unsigned int nested_level)
    : Grid(static_cast<const Grid&>(parent))
    , m_constraints()
    , m_alternatives()
    , m_line_completed()
    , m_line_to_be_reduced()
    , m_nb_alternatives()
    , m_grid_stats(nullptr)
    , m_observer(parent.m_observer)
    , m_abort_function(parent.m_abort_function)
    , m_max_nb_alternatives(LineSelectionPolicy::initial_max_nb_alternatives())
    , m_branching_depth(nested_level)
    , m_binomial(parent.m_binomial)
{
    assert(nested_level > 0u);
    assert(m_binomial);

    m_constraints[Line::ROW] = parent.m_constraints[Line::ROW];
    m_constraints[Line::COL] = parent.m_constraints[Line::COL];
    m_alternatives[Line::ROW] = build_line_alternatives_from(Line::ROW, m_constraints[Line::ROW], static_cast<const Grid&>(*this), *m_binomial);
    m_alternatives[Line::COL] = build_line_alternatives_from(Line::COL, m_constraints[Line::COL], static_cast<const Grid&>(*this), *m_binomial);
    m_line_completed[Line::ROW] = parent.m_line_completed[Line::ROW];
    m_line_completed[Line::COL] = parent.m_line_completed[Line::COL];
    m_line_to_be_reduced[Line::ROW] = parent.m_line_to_be_reduced[Line::ROW];
    m_line_to_be_reduced[Line::COL] = parent.m_line_to_be_reduced[Line::COL];
    m_nb_alternatives[Line::ROW] = parent.m_nb_alternatives[Line::ROW];
    m_nb_alternatives[Line::COL] = parent.m_nb_alternatives[Line::COL];

    // Solver::Observer
    if (m_observer)
    {
        m_observer(Solver::Event::BRANCHING, nullptr, nested_level);
    }
}

template <typename LineSelectionPolicy, bool BranchingAllowed>
void WorkGrid<LineSelectionPolicy, BranchingAllowed>::set_stats(GridStats* stats)
{
    this->m_grid_stats = stats;
    if (stats)
        stats->max_branching_depth = m_branching_depth;
}


template <typename LineSelectionPolicy, bool BranchingAllowed>
Solver::Status WorkGrid<LineSelectionPolicy, BranchingAllowed>::line_solve(Solver::Solutions& solutions)
{
    if (m_branching_depth == 0u)
    {
        const auto pass_status = full_grid_initial_pass();

        if (pass_status.contradictory)
            return Solver::Status::CONTRADICTORY_GRID;
    }

    bool grid_completed = all_lines_completed();

    // While the reduce method is making progress, call it!
    while (!grid_completed)
    {
        const auto pass_status = full_grid_pass();
        if (pass_status.contradictory)
            return Solver::Status::CONTRADICTORY_GRID;

        grid_completed = all_lines_completed();

        // Exit loop either if the grid has completed or if the condition to switch the branching search is met
        if (LineSelectionPolicy::switch_to_branching(m_max_nb_alternatives, pass_status.grid_changed, pass_status.skipped_lines, m_branching_depth))
            break;

        // Max number of alternatives for the next full grid pass
        m_max_nb_alternatives = LineSelectionPolicy::get_max_nb_alternatives(m_max_nb_alternatives, pass_status.grid_changed, pass_status.skipped_lines, m_branching_depth);
    }

    // Are we done?
    if (grid_completed)
    {
        if (m_observer)
        {
            m_observer(Solver::Event::SOLVED_GRID, nullptr, m_branching_depth);
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
                for (unsigned int idx = 0u; idx < m_nb_alternatives[type].size(); idx++)
                {
                    const auto nb_alt = m_line_to_be_reduced[type][idx] ? 0u : m_nb_alternatives[type][idx];
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
                const LineConstraint& line_constraint = m_constraints[found_line_type].at(found_line_index);
                const Line& known_tiles = get_line(found_line_type, found_line_index);

                m_guess_list_of_all_alternatives = line_constraint.build_all_possible_lines(known_tiles);
                assert(m_guess_list_of_all_alternatives.size() == min_alt);

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
            UNUSED(max_nb_solutions);
            // Store the incomplete solution
            save_solution(solutions);
        }
    }

    return status;
}


template <typename LineSelectionPolicy, bool BranchingAllowed>
bool WorkGrid<LineSelectionPolicy, BranchingAllowed>::all_lines_completed() const
{
    const bool all_rows = std::all_of(std::cbegin(m_line_completed[Line::ROW]), std::cend(m_line_completed[Line::ROW]), [](bool b) { return b; });
    const bool all_cols = std::all_of(std::cbegin(m_line_completed[Line::COL]), std::cend(m_line_completed[Line::COL]), [](bool b) { return b; });

    // The logical AND is important here: in the case an hypothesis is made on a row (resp. a column), it is marked as completed
    // but the constraints on the columns (resp. the rows) may not be all satisfied.
    return all_rows & all_cols;
}


template <typename LineSelectionPolicy, bool BranchingAllowed>
bool WorkGrid<LineSelectionPolicy, BranchingAllowed>::set_line(const Line& line, unsigned int nb_alt)
{
    static const Line DEFAULT_LINE(Line::ROW, 0, 0);
    const auto line_type = line.type();
    const size_t line_index = line.index();
    const Line observer_original_line = m_observer ? get_line(line.type(), line_index) : DEFAULT_LINE;
    assert(line.size() == static_cast<unsigned int>(line.type() == Line::ROW ? width() : height()));
    const Line::Container& tiles = line.tiles();

    bool line_changed = false;
    const auto set_tile_func = [this, &line_changed](Line::Type type, unsigned int idx) {
        // mark the impacted line or column with flag "to be reduced"
        m_line_to_be_reduced[type][idx] = true;
        line_changed = true;
    };

    bool line_is_complete = true;
    if (line_type == Line::ROW)
    {
        for (auto tile_index = 0u; tile_index < line.size(); tile_index++)
        {
            const bool tile_changed = set(tile_index, line_index, tiles[tile_index]);
            line_is_complete &= (get(tile_index, line_index) != Tile::UNKNOWN);
            if (tile_changed)
                set_tile_func(Line::COL, tile_index);
        }
    }
    else
    {
        for (auto tile_index = 0u; tile_index < line.size(); tile_index++)
        {
            const bool tile_changed = set(line_index, tile_index, tiles[tile_index]);
            line_is_complete &= (get(line_index, tile_index) != Tile::UNKNOWN);
            if (tile_changed)
                set_tile_func(Line::ROW, tile_index);
        }
    }

    if (nb_alt > 0) { m_nb_alternatives[line_type][line_index] = nb_alt; }

    m_line_completed[line_type][line_index] = line_is_complete;

    // Most of the time after a line is set, it does not need to be reduced another time
    // Exceptions to the rule must be handled by the caller
    m_line_to_be_reduced[line_type][line_index] = false;

    if (m_observer && line_changed)
    {
        const Line delta = line_delta(observer_original_line, get_line(line_type, line_index));
        m_observer(Solver::Event::DELTA_LINE, &delta, m_branching_depth);
    }
    return line_changed;
}


// On the first pass of the grid, assume that the color of every tile is unknown and compute the trivial reduction and theoritical number of alternatives
template <typename LineSelectionPolicy, bool BranchingAllowed>
typename WorkGrid<LineSelectionPolicy, BranchingAllowed>::PassStatus WorkGrid<LineSelectionPolicy, BranchingAllowed>::single_line_initial_pass(Line::Type type, unsigned int index)
{
    PassStatus status;
    const LineConstraint& constraint = m_constraints[type].at(index);

    const auto line_size = static_cast<unsigned int>(type == Line::ROW ? width() : height());

    // Compute the trivial reduction if it exists and the number of alternatives
    assert(m_binomial);
    const auto nb_alt = constraint.line_trivial_nb_alternatives(line_size, *m_binomial);
    const auto reduced_line = constraint.line_trivial_reduction(line_size, index);

    if (m_grid_stats != nullptr)
    {
        m_grid_stats->max_initial_nb_alternatives = std::max(m_grid_stats->max_initial_nb_alternatives, nb_alt);
    }

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
        // Here since we are computing a trivial reduction assuming the initial line is completely unknown we
        // are ignoring tiles that are possibly already set. In such a case, we need to redo a reduction
        // on the next full grid pass.
        if (reduced_line != get_line(type, index))
        {
            m_line_to_be_reduced[type][index] = !m_line_completed[type][index];
        }
    }

    return status;
}


template <typename LineSelectionPolicy, bool BranchingAllowed>
typename WorkGrid<LineSelectionPolicy, BranchingAllowed>::PassStatus WorkGrid<LineSelectionPolicy, BranchingAllowed>::single_line_pass(Line::Type type, unsigned int index)
{
    assert(m_line_completed[type][index] == false);
    assert(m_line_to_be_reduced[type][index] == true);
    PassStatus status;
    if (m_grid_stats != nullptr) { m_grid_stats->nb_single_line_pass_calls++; }

    // Reduce all possible lines that match the data already present in the grid and the line constraint
    if (m_grid_stats != nullptr) { m_grid_stats->nb_reduce_and_count_alternatives_calls++; }
    const auto full_reduction = m_alternatives[type][index].full_reduction();

    // If the list of alternative lines is empty, it means the grid data is contradictory
    if (full_reduction.nb_alternatives == 0)
    {
        status.contradictory = true;
        return status;
    }
    if (m_grid_stats != nullptr) { m_grid_stats->max_nb_alternatives = std::max(m_grid_stats->max_nb_alternatives, full_reduction.nb_alternatives); }

    // In any case, update the grid data with the reduced line resulting from the list of alternatives
    status.grid_changed = set_line(full_reduction.reduced_line, full_reduction.nb_alternatives);

    if (m_grid_stats != nullptr && status.grid_changed)
    {
        m_grid_stats->nb_single_line_pass_calls_w_change++;
        m_grid_stats->max_nb_alternatives_w_change = std::max(m_grid_stats->max_nb_alternatives_w_change, full_reduction.nb_alternatives);
    }

    return status;
}


// Reduce all rows or all columns. Return false if no change was made on the grid.
template <typename LineSelectionPolicy, bool BranchingAllowed>
typename WorkGrid<LineSelectionPolicy, BranchingAllowed>::PassStatus WorkGrid<LineSelectionPolicy, BranchingAllowed>::full_side_pass(Line::Type type)
{
    PassStatus status;
    const auto length = type == Line::ROW ? height() : width();

    for (unsigned int x = 0u; x < length; x++)
    {
        if (m_line_to_be_reduced[type][x])
        {
            if (m_nb_alternatives[type][x] <= m_max_nb_alternatives)
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
        if (m_abort_function && m_abort_function())
            throw PicrossSolverAborted();
    }

    return status;
}

template <typename LineSelectionPolicy, bool BranchingAllowed>
typename WorkGrid<LineSelectionPolicy, BranchingAllowed>::PassStatus WorkGrid<LineSelectionPolicy, BranchingAllowed>::full_grid_initial_pass()
{
    PassStatus status;

    // Pass on columns
    const auto w = width();
    for (unsigned int x = 0u; x < w; x++)
    {
        status += single_line_initial_pass(Line::COL, x);
        if (status.contradictory)
            return status;
    }

    // Pass on rows
    const auto h = height();
    for (unsigned int y = 0u; y < h; y++)
    {
        status += single_line_initial_pass(Line::ROW, y);
        if (status.contradictory)
            return status;
    }

    if (m_abort_function && m_abort_function())
        throw PicrossSolverAborted();

    return status;
}

// Reduce all columns and all rows. Return false if no change was made on the grid.
// Return true if the grid was changed during the full pass
template <typename LineSelectionPolicy, bool BranchingAllowed>
typename WorkGrid<LineSelectionPolicy, BranchingAllowed>::PassStatus WorkGrid<LineSelectionPolicy, BranchingAllowed>::full_grid_pass()
{
    PassStatus status;
    if (m_grid_stats != nullptr) { m_grid_stats->nb_full_grid_pass_calls++; }

    // Pass on columns
    status += full_side_pass(Line::COL);
    if (status.contradictory)
        return status;

    // Pass on rows
    status += full_side_pass(Line::ROW);
    return status;
}


template <typename LineSelectionPolicy, bool BranchingAllowed>
Solver::Status WorkGrid<LineSelectionPolicy, BranchingAllowed>::branch(Solver::Solutions& solutions, unsigned int max_nb_solutions) const
{
    assert(m_guess_list_of_all_alternatives.size() > 0u);
    /* This function will test a range of alternatives for one particular line of the grid, each time
     * creating a new instance of the grid class on which the function WorkGrid<LineSelectionPolicy, BranchingAllowed>::solve() is called.
     */

    if (m_grid_stats != nullptr)
    {
        m_grid_stats->nb_branching_calls++;
        const auto nb_alts = static_cast<unsigned int>(m_guess_list_of_all_alternatives.size());
        m_grid_stats->total_nb_branching_alternatives += nb_alts;
        if (m_grid_stats->max_nb_alternatives_by_branching_depth.size() < m_branching_depth + 1)
            m_grid_stats->max_nb_alternatives_by_branching_depth.resize(m_branching_depth + 1, 0u);
        auto& max_nb_alts = m_grid_stats->max_nb_alternatives_by_branching_depth[m_branching_depth];
        max_nb_alts = std::max(max_nb_alts, nb_alts);
    }

    bool flag_solution_found = false;
    for (const Line& guess_line : m_guess_list_of_all_alternatives)
    {
        // Allocate a new work grid. Use the shallow copy.
        WorkGrid new_grid(*this, m_branching_depth + 1);
        Solver::Solutions nested_solutions;
        std::unique_ptr<GridStats> nested_stats;
        if (m_grid_stats)
            nested_stats = std::make_unique<GridStats>();

        // Set one line in the new_grid according to the hypothesis we made. That line is then complete
        const bool changed = new_grid.set_line(guess_line, 1u);
        assert(changed);

        // Solve the new grid!
        new_grid.set_stats(nested_stats.get());
        const auto status = new_grid.solve(nested_solutions, max_nb_solutions);

        if (m_grid_stats)
        {
            assert(nested_stats);
            merge_nested_grid_stats(*m_grid_stats, *nested_stats);
        }

        flag_solution_found |= status == Solver::Status::OK;

        solutions.reserve(solutions.size() + nested_solutions.size());
        std::move(nested_solutions.begin(), nested_solutions.end(), std::back_inserter(solutions));
        nested_solutions.clear();       // Entries are now invalidated

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
    for (unsigned int x = 0u; x < width(); x++) { valid &= m_constraints[Line::COL][x].compatible(get_line<Line::COL>(x)); }
    for (unsigned int y = 0u; y < height(); y++) { valid &= m_constraints[Line::ROW][y].compatible(get_line<Line::ROW>(y)); }
    return valid;
}


template <typename LineSelectionPolicy, bool BranchingAllowed>
void WorkGrid<LineSelectionPolicy, BranchingAllowed>::save_solution(Solver::Solutions& solutions) const
{
    if constexpr (BranchingAllowed)
    {
        assert(valid_solution());
        if (m_grid_stats != nullptr) { m_grid_stats->nb_solutions++; }
    }
    else
    {
        assert(m_branching_depth == 0);
    }

    // Shallow copy of only the grid data
    solutions.emplace_back(Solver::Solution{ static_cast<const OutputGrid&>(*this), m_branching_depth });
}


// Template explicit instantiations
template class WorkGrid<LineSelectionPolicy_Legacy, true>;
template class WorkGrid<LineSelectionPolicy_RampUpMaxNbAlternatives, true>;
template class WorkGrid<LineSelectionPolicy_RampUpMaxNbAlternatives, false>;

} // namespace picross
