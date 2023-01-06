/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2022 Pierre DEJOUE
 ******************************************************************************/
#include "work_grid.h"

#include "line.h"
#include "macros.h"
#include "picross_stats_internal.h"
#include "solver_policy.h"

#include <picross/picross.h>

#include <algorithm>
#include <cassert>
#include <exception>
#include <iterator>
#include <memory>


namespace picross
{

// Exception returned by WorkGrid::solve() if the processing was aborted from the outside
class PicrossSolverAborted : public std::exception
{};

namespace
{
constexpr bool PARTIAL_SOLUTION = true;
constexpr bool FULL_SOLUTION = false;

std::vector<LineConstraint> build_constraints_from(Line::Type type, const InputGrid& grid)
{
    std::vector<LineConstraint> output;
    const InputGrid::Constraints& input = type == Line::ROW ? grid.m_rows : grid.m_cols;
    output.reserve(input.size());
    std::transform(input.cbegin(), input.cend(), std::back_inserter(output), [type](const auto& c) { return LineConstraint(type, c); });
    return output;
}

std::vector<LineAlternatives> build_line_alternatives_from(Line::Type type, const std::vector<LineConstraint>& constraints, const Grid& grid, BinomialCoefficients::Cache& binomial)
{
    std::vector<LineAlternatives> output;
    output.reserve(constraints.size());
    Line::Index idx = 0;
    std::transform(constraints.cbegin(), constraints.cend(), std::back_inserter(output), [type, &grid, &idx, &binomial](const auto& c) { return LineAlternatives(c, grid.get_line(type, idx++), binomial); });
    return output;
}

std::vector<LineAlternatives> export_line_alternatives_to_new_grid(Line::Type type, const std::vector<LineAlternatives>& alternatives, const Grid& new_grid)
{
    std::vector<LineAlternatives> output;
    output.reserve(alternatives.size());
    Line::Index idx = 0;
    std::transform(alternatives.cbegin(), alternatives.cend(), std::back_inserter(output), [type, &new_grid, &idx](const auto& alt) { return LineAlternatives(alt, new_grid.get_line(type, idx++)); });
    return output;
}

template <bool B>
void update_line_range(LineRange& range, const std::vector<bool>& source)
{
    assert(range.m_begin <= range.m_end);
    assert(0 <= range.m_begin && (range.empty() || (range.m_begin < source.size())));
    assert(0 <= range.m_end && range.m_end <= source.size());
    while (range.m_begin < range.m_end && source[range.m_begin] == B)    { range.m_begin++; }
    while (range.m_begin < range.m_end && source[range.m_end - 1u] == B) { range.m_end--; }
    assert(range.m_begin <= range.m_end);
}
}  // namespace


template <typename SolverPolicy>
WorkGrid<SolverPolicy>::WorkGrid(const InputGrid& grid, const SolverPolicy& solver_policy, Solver::Observer observer, Solver::Abort abort_function)
    : Grid(grid.width(), grid.height(), grid.name())
    , m_state(State::INITIAL_PASS)
    , m_solver_policy(solver_policy)
    , m_constraints()
    , m_alternatives()
    , m_line_completed()
    , m_line_is_fully_reduced()
    , m_nb_alternatives()
    , m_uncompleted_lines_range()
    , m_all_lines()
    , m_uncompleted_lines_end(m_all_lines.end())
    , m_grid_stats(nullptr)
    , m_observer(std::move(observer))
    , m_abort_function(std::move(abort_function))
    , m_max_nb_alternatives(solver_policy.m_min_nb_alternatives)
    , m_branching_depth(0u)
    , m_binomial(std::make_shared<BinomialCoefficients::Cache>())
{
    assert(m_binomial);

    m_all_lines.reserve(width() + height());
    for (Line::Index row_idx = 0; row_idx < height(); row_idx++)
    {
        m_all_lines.emplace_back(Line::ROW, row_idx);
    }
    for (Line::Index col_idx = 0; col_idx < width(); col_idx++)
    {
        m_all_lines.emplace_back(Line::COL, col_idx);
    }
    m_uncompleted_lines_end = m_all_lines.end();

    m_constraints[Line::ROW] = build_constraints_from(Line::ROW, grid);
    m_constraints[Line::COL] = build_constraints_from(Line::COL, grid);
    m_alternatives[Line::ROW] = build_line_alternatives_from(Line::ROW, m_constraints[Line::ROW], static_cast<const Grid&>(*this), *m_binomial);
    m_alternatives[Line::COL] = build_line_alternatives_from(Line::COL, m_constraints[Line::COL], static_cast<const Grid&>(*this), *m_binomial);
    m_line_completed[Line::ROW].resize(height(), false);
    m_line_completed[Line::COL].resize(width(), false);
    m_line_has_updates[Line::ROW].resize(height(), false);
    m_line_has_updates[Line::COL].resize(width(), false);
    m_line_is_fully_reduced[Line::ROW].resize(height(), false);
    m_line_is_fully_reduced[Line::COL].resize(width(), false);
    m_nb_alternatives[Line::ROW].resize(height(), 0u);
    m_nb_alternatives[Line::COL].resize(width(), 0u);
    m_uncompleted_lines_range[Line::ROW] = { 0u, static_cast<Line::Index>(height()) };
    m_uncompleted_lines_range[Line::COL] = { 0u, static_cast<Line::Index>(width()) };

    assert(m_constraints[Line::ROW].size() == height());
    assert(m_constraints[Line::COL].size() == width());
}


template <typename SolverPolicy>
WorkGrid<SolverPolicy>::WorkGrid(const WorkGrid& parent, const SolverPolicy& solver_policy, State initial_state)
    : Grid(parent)
    , m_state(initial_state)
    , m_solver_policy(solver_policy)
    , m_constraints()
    , m_alternatives()
    , m_line_completed()
    , m_line_is_fully_reduced()
    , m_nb_alternatives()
    , m_uncompleted_lines_range()
    , m_all_lines(parent.m_all_lines)
    , m_uncompleted_lines_end(m_all_lines.end())
    , m_grid_stats(nullptr)
    , m_observer(parent.m_observer)
    , m_abort_function(parent.m_abort_function)
    , m_max_nb_alternatives(solver_policy.m_min_nb_alternatives)
    , m_branching_depth(parent.m_branching_depth + 1)
    , m_binomial(parent.m_binomial)
{
    assert(m_binomial);

    m_constraints[Line::ROW] = parent.m_constraints[Line::ROW];
    m_constraints[Line::COL] = parent.m_constraints[Line::COL];
    m_alternatives[Line::ROW] = export_line_alternatives_to_new_grid(Line::ROW, parent.m_alternatives[Line::ROW], *this);
    m_alternatives[Line::COL] = export_line_alternatives_to_new_grid(Line::COL, parent.m_alternatives[Line::COL], *this);
    m_line_completed[Line::ROW] = parent.m_line_completed[Line::ROW];
    m_line_completed[Line::COL] = parent.m_line_completed[Line::COL];
    m_line_has_updates[Line::ROW] = parent.m_line_has_updates[Line::ROW];
    m_line_has_updates[Line::COL] = parent.m_line_has_updates[Line::COL];
    m_line_is_fully_reduced[Line::ROW] = parent.m_line_is_fully_reduced[Line::ROW];
    m_line_is_fully_reduced[Line::COL] = parent.m_line_is_fully_reduced[Line::COL];
    m_nb_alternatives[Line::ROW] = parent.m_nb_alternatives[Line::ROW];
    m_nb_alternatives[Line::COL] = parent.m_nb_alternatives[Line::COL];
    m_uncompleted_lines_range[Line::ROW] = parent.m_uncompleted_lines_range[Line::ROW];
    m_uncompleted_lines_range[Line::COL] = parent.m_uncompleted_lines_range[Line::COL];
    m_uncompleted_lines_end = m_all_lines.begin() + std::distance(parent.m_all_lines.begin(), AllLines::const_iterator(parent.m_uncompleted_lines_end));
}


template <typename SolverPolicy>
void WorkGrid<SolverPolicy>::set_stats(GridStats* stats)
{
    this->m_grid_stats = stats;
    if (stats)
        stats->max_branching_depth = m_branching_depth;
}


template <typename SolverPolicy>
Solver::Status WorkGrid<SolverPolicy>::line_solve(const Solver::SolutionFound& solution_found)
{
    Solver::Status status = Solver::Status::OK;
    try
    {
        bool grid_completed = false;
        PassStatus pass_status;

        while (m_state != State::BRANCHING && !grid_completed)
        {
            if (m_observer)
            {
                m_observer(Solver::Event::INTERNAL_STATE, nullptr, m_branching_depth, static_cast<unsigned int>(m_state));
            }

            switch (m_state)
            {
            case State::INITIAL_PASS:
                pass_status = full_grid_pass<State::INITIAL_PASS>();
                m_state = State::PARTIAL_REDUCTION;
                break;

            case State::PARTIAL_REDUCTION:
                pass_status = full_grid_pass<State::PARTIAL_REDUCTION>();
                if (!pass_status.grid_changed)
                {
                    m_state = State::FULL_REDUCTION;
                    sort_by_nb_alternatives();
                }
                break;

            case State::FULL_REDUCTION:
                pass_status = full_grid_pass<State::FULL_REDUCTION>();
                if (m_solver_policy.switch_to_branching(m_max_nb_alternatives, pass_status.grid_changed, pass_status.skipped_lines))
                {
                    m_state = State::BRANCHING;
                }
                else
                {
                    // Max number of alternatives for the next full grid pass
                    m_max_nb_alternatives = m_solver_policy.get_max_nb_alternatives(m_max_nb_alternatives, pass_status.grid_changed, pass_status.skipped_lines);
                    if (!pass_status.grid_changed)
                        m_state = State::PARTIAL_REDUCTION;
                }
                break;

            case State::BRANCHING:
            default:
                assert(0);
                break;
            }

            if (pass_status.contradictory)
                break;

            grid_completed = all_lines_completed();
        }

        // Are we done?
        if (grid_completed)
        {
            const bool cont = found_solution(solution_found);
            status = cont ? Solver::Status::OK : Solver::Status::ABORTED;
        }
        // If we are not, the grid is either contradictory or not line solvable
        else if (pass_status.contradictory)
        {
            status = Solver::Status::CONTRADICTORY_GRID;
        }
        else
        {
            status = Solver::Status::NOT_LINE_SOLVABLE;
        }
    }
    catch (const PicrossSolverAborted&)
    {
        status = Solver::Status::ABORTED;
    }
    return status;
}


template <typename SolverPolicy>
Solver::Status WorkGrid<SolverPolicy>::solve(const Solver::SolutionFound& solution_found)
{
    auto status = line_solve(solution_found);

    if (status == Solver::Status::NOT_LINE_SOLVABLE)
    {
        if (m_solver_policy.m_branching_allowed)
        {
            assert(m_state == State::BRANCHING);

            // Make a guess
            status = branch(solution_found);
        }
        else if (m_branching_depth == 0)
        {
            // Partial solution
            solution_found(Solver::Solution{ OutputGrid(*this), m_branching_depth, PARTIAL_SOLUTION });
        }
    }

    return status;
}


template <typename SolverPolicy>
bool WorkGrid<SolverPolicy>::all_lines_completed() const
{
    const bool all_completed = (m_all_lines.begin() == m_uncompleted_lines_end);
    assert(all_completed == [this]() -> bool {
        const bool all_rows = std::all_of(std::cbegin(m_line_completed[Line::ROW]), std::cend(m_line_completed[Line::ROW]), [](bool b) { return b; });
        const bool all_cols = std::all_of(std::cbegin(m_line_completed[Line::COL]), std::cend(m_line_completed[Line::COL]), [](bool b) { return b; });
        return all_rows & all_cols;
    }());
    return all_completed;
}


template <typename SolverPolicy>
bool WorkGrid<SolverPolicy>::update_line(const Line& line, unsigned int nb_alt)
{
    assert(nb_alt > 0);
    static const Line DEFAULT_LINE(Line::ROW, 0, 0);
    const auto line_type = line.type();
    const auto line_index = line.index();
    const auto line_sz = line.size();
    const Line observer_original_line = m_observer ? line_from_line_span(get_line(line.type(), line_index)) : DEFAULT_LINE;
    assert(line.size() == static_cast<unsigned int>(line.type() == Line::ROW ? width() : height()));
    const Line::Container& tiles = line.tiles();

    bool line_changed = false;
    const auto set_tile_func = [this, &line_changed](Line::Type type, unsigned int idx) {
        m_line_has_updates[type][idx] = true;
        // mark the impacted line or column as "to be reduced"
        m_line_is_fully_reduced[type][idx] = false;
        line_changed = true;
    };

    bool line_is_complete = true;
    const LineSpan grid_line = get_line(line_type, line_index);
    if (line_type == Line::ROW)
    {
        for (Line::Index tile_idx = 0u; tile_idx < line_sz; tile_idx++)
        {
            const bool tile_changed = update(tile_idx, line_index, tiles[tile_idx]);
            line_is_complete &= (grid_line[tile_idx] != Tile::UNKNOWN);
            if (tile_changed)
                set_tile_func(Line::COL, tile_idx);
        }
    }
    else
    {
        assert(line_type == Line::COL);
        for (Line::Index tile_idx = 0u; tile_idx < line_sz; tile_idx++)
        {
            const bool tile_changed = update(line_index, tile_idx, tiles[tile_idx]);
            line_is_complete &= (grid_line[tile_idx] != Tile::UNKNOWN);
            if (tile_changed)
                set_tile_func(Line::ROW, tile_idx);
        }
    }

    m_nb_alternatives[line_type][line_index] = nb_alt = line_is_complete ? 1u : nb_alt;
    m_line_completed[line_type][line_index] = line_is_complete;
    m_line_has_updates[line_type][line_index] = line_changed;

    if (m_observer && line_changed)
    {
        const Line delta = get_line(line_type, line_index) - observer_original_line;
        m_observer(Solver::Event::DELTA_LINE, &delta, m_branching_depth, nb_alt);
    }

    return line_changed;
}


template <typename SolverPolicy>
void WorkGrid<SolverPolicy>::partition_completed_lines()
{
    m_uncompleted_lines_end = std::partition(m_all_lines.begin(), m_uncompleted_lines_end, [this](const LineId id) { return !m_line_completed[id.m_type][id.m_index]; });
    update_line_range<true>(m_uncompleted_lines_range[Line::ROW], m_line_completed[Line::ROW]);
    update_line_range<true>(m_uncompleted_lines_range[Line::COL], m_line_completed[Line::COL]);
}


template <typename SolverPolicy>
void WorkGrid<SolverPolicy>::sort_by_nb_alternatives()
{
    std::sort(m_all_lines.begin(), m_uncompleted_lines_end, [this](const auto& lhs, const auto& rhs) {
        return m_nb_alternatives[lhs.m_type][lhs.m_index] < m_nb_alternatives[rhs.m_type][rhs.m_index];
    });
}

template <typename SolverPolicy>
typename WorkGrid<SolverPolicy>::PassStatus WorkGrid<SolverPolicy>::single_line_initial_pass(Line::Type type, unsigned int index)
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
    status.grid_changed = update_line(reduced_line, nb_alt);

    return status;
}


template <typename SolverPolicy>
typename WorkGrid<SolverPolicy>::PassStatus WorkGrid<SolverPolicy>::single_line_partial_reduction(Line::Type type, unsigned int index)
{
    PassStatus status;

    if (!m_line_has_updates[type][index] || m_line_is_fully_reduced[type][index])
    {
        return status;
    }

    // Reduce all possible lines that match the data already present in the grid and the line constraint
    if (m_grid_stats != nullptr) { m_grid_stats->nb_single_line_partial_reduction++; }
    const auto partial_reduction = m_alternatives[type][index].partial_reduction(1);

    // If the list of alternative lines is empty, it means the grid data is contradictory
    if (partial_reduction.nb_alternatives == 0)
    {
        status.contradictory = true;
        return status;
    }

    // In any case, update the grid data with the reduced line resulting from the list of alternatives
    const auto nb_alternatives = std::min(partial_reduction.nb_alternatives, m_nb_alternatives[type][index]);
    const bool line_changed = status.grid_changed = update_line(partial_reduction.reduced_line, nb_alternatives);
    if (line_changed)
        m_line_is_fully_reduced[type][index] = false;
    if (partial_reduction.is_fully_reduced)
        m_line_is_fully_reduced[type][index] = true;

    if (m_grid_stats != nullptr && line_changed)
    {
        m_grid_stats->nb_single_line_partial_reduction_w_change++;
    }

    return status;
}


template <typename SolverPolicy>
typename WorkGrid<SolverPolicy>::PassStatus WorkGrid<SolverPolicy>::single_line_full_reduction(Line::Type type, unsigned int index)
{
    PassStatus status;

    if (m_line_is_fully_reduced[type][index] || m_nb_alternatives[type][index] > m_max_nb_alternatives)
    {
        if (!m_line_is_fully_reduced[type][index])
            status.skipped_lines++;
        return status;
    }

    // Reduce all possible lines that match the data already present in the grid and the line constraint
    if (m_grid_stats != nullptr) { m_grid_stats->nb_single_line_full_reduction++; }
    const auto full_reduction = m_alternatives[type][index].full_reduction();

    // If the list of alternative lines is empty, it means the grid data is contradictory
    if (full_reduction.nb_alternatives == 0)
    {
        status.contradictory = true;
        return status;
    }
    if (m_grid_stats != nullptr) { m_grid_stats->max_nb_alternatives = std::max(m_grid_stats->max_nb_alternatives, full_reduction.nb_alternatives); }

    // In any case, update the grid data with the reduced line resulting from the list of alternatives
    status.grid_changed = update_line(full_reduction.reduced_line, full_reduction.nb_alternatives);

    assert(full_reduction.is_fully_reduced);
    m_line_is_fully_reduced[type][index] = true;

    if (m_grid_stats != nullptr && status.grid_changed)
    {
        m_grid_stats->nb_single_line_full_reduction_w_change++;
        m_grid_stats->max_nb_alternatives_w_change = std::max(m_grid_stats->max_nb_alternatives_w_change, full_reduction.nb_alternatives);
    }

    return status;
}


// Reduce all columns and all rows. Return false if no change was made on the grid.
// Return true if the grid was changed during the full pass
template <typename SolverPolicy>
template <typename WorkGrid<SolverPolicy>::State S>
typename WorkGrid<SolverPolicy>::PassStatus WorkGrid<SolverPolicy>::full_grid_pass()
{
    PassStatus status;
    if (m_grid_stats != nullptr) { m_grid_stats->nb_full_grid_pass++; }

    for(auto it = m_all_lines.begin(); it != m_uncompleted_lines_end; ++it)
    {
        if (m_line_completed[it->m_type][it->m_index])
            continue;
        if constexpr (S == State::INITIAL_PASS)
        {
            status += single_line_initial_pass(it->m_type, it->m_index);
        }
        else if constexpr (S == State::PARTIAL_REDUCTION)
        {
            status += single_line_partial_reduction(it->m_type, it->m_index);
        }
        else
        {
            static_assert(S == State::FULL_REDUCTION);
            status += single_line_full_reduction(it->m_type, it->m_index);
        }
        if (status.contradictory)
            break;
        if (m_abort_function && m_abort_function())
            throw PicrossSolverAborted();
    }
    if constexpr (S == State::INITIAL_PASS)
    {
        for(auto it = m_all_lines.begin(); it != m_uncompleted_lines_end; ++it)
            m_line_has_updates[it->m_type][it->m_index] = true;
    }
    partition_completed_lines();
    return status;
}

// This method will test a range of alternatives for one particular line of the grid, each time
// creating a new instance of the grid class on which the function WorkGrid<SolverPolicy>::solve() is called.
template <typename SolverPolicy>
Solver::Status WorkGrid<SolverPolicy>::branch(const Solver::SolutionFound& solution_found)
{
    assert(m_solver_policy.m_branching_allowed);
    assert(m_all_lines.begin() != m_uncompleted_lines_end);

    // Find the row or column not yet solved with the minimal alternative lines.
    sort_by_nb_alternatives();
    const LineId& found_line = m_all_lines.front();
    const LineConstraint& line_constraint = m_constraints[found_line.m_type][found_line.m_index];
    const LineSpan& known_tiles = get_line(found_line.m_type, found_line.m_index);
    const auto nb_alt = m_nb_alternatives[found_line.m_type][found_line.m_index];
    assert(nb_alt >= 2);

    // Build all alternatives for that row or column
    const auto list_of_all_alternatives = line_constraint.build_all_possible_lines(known_tiles);
    assert(list_of_all_alternatives.size() == nb_alt);
    assert(!list_of_all_alternatives.empty());  // Then the grid would be contradictory, but this must be catched earlier

    if (m_observer)
    {
        const auto line_known_tiles = line_from_line_span(known_tiles);
        m_observer(Solver::Event::BRANCHING, &line_known_tiles, m_branching_depth, nb_alt);
    }

    if (m_grid_stats != nullptr)
    {
        m_grid_stats->nb_branching_calls++;
        const auto nb_alts = static_cast<unsigned int>(list_of_all_alternatives.size());
        m_grid_stats->total_nb_branching_alternatives += nb_alts;
        if (m_grid_stats->max_nb_alternatives_by_branching_depth.size() < m_branching_depth + 1)
            m_grid_stats->max_nb_alternatives_by_branching_depth.resize(m_branching_depth + 1, 0u);
        auto& max_nb_alts = m_grid_stats->max_nb_alternatives_by_branching_depth[m_branching_depth];
        max_nb_alts = std::max(max_nb_alts, nb_alts);
    }

    Solver::Status status = Solver::Status::OK;
    bool flag_solution_found = false;
    for (const Line& guess_line : list_of_all_alternatives)
    {
        // Copy current grid state to a nested grid
        WorkGrid<SolverPolicy> nested_work_grid(*this, m_solver_policy, State::PARTIAL_REDUCTION);
        if (m_observer)
        {
            m_observer(Solver::Event::BRANCHING, nullptr, nested_work_grid.m_branching_depth, 0);
        }

        // Set one line in the new_grid according to the hypothesis we made. That line is then complete
        const bool changed = nested_work_grid.update_line(guess_line, 1u);
        assert(changed);
        nested_work_grid.m_line_is_fully_reduced[guess_line.type()][guess_line.index()] = true;
        assert(nested_work_grid.m_line_completed[guess_line.type()][guess_line.index()]);
        nested_work_grid.partition_completed_lines();

        // Solve the new grid!
        Solver::Solutions nested_solutions;
        std::unique_ptr<GridStats> nested_stats = m_grid_stats ? std::make_unique<GridStats>() : nullptr;
        nested_work_grid.set_stats(nested_stats.get());
        status = nested_work_grid.solve(solution_found);

        if (m_grid_stats)
        {
            assert(nested_stats);
            merge_branching_grid_stats(*m_grid_stats, *nested_stats);
        }

        flag_solution_found |= (status == Solver::Status::OK);

        if (status == Solver::Status::ABORTED)
            return status;
    }

    return flag_solution_found ? Solver::Status::OK : Solver::Status::CONTRADICTORY_GRID;
}


template <typename SolverPolicy>
bool WorkGrid<SolverPolicy>::is_valid_solution() const
{
    assert(is_solved());
    bool valid = true;
    for (unsigned int x = 0u; x < width(); x++)  { valid &= m_constraints[Line::COL][x].compatible(get_line(Line::COL, x)); }
    for (unsigned int y = 0u; y < height(); y++) { valid &= m_constraints[Line::ROW][y].compatible(get_line(Line::ROW, y)); }
    return valid;
}


template <typename SolverPolicy>
bool WorkGrid<SolverPolicy>::found_solution(const Solver::SolutionFound& solution_found) const
{
    assert(is_valid_solution());
    if (m_grid_stats != nullptr) { m_grid_stats->nb_solutions++; }
    if (m_observer) { m_observer(Solver::Event::SOLVED_GRID, nullptr, m_branching_depth, 0); }

    // Shallow copy of only the grid data
    return solution_found(Solver::Solution{ OutputGrid(*this), m_branching_depth, FULL_SOLUTION });
}

// Explicit template instantiations
template class WorkGrid<SolverPolicy_RampUpMaxNbAlternatives>;

} // namespace picross
