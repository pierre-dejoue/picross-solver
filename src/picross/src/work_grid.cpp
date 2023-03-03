/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2022 Pierre DEJOUE
 ******************************************************************************/
#include "work_grid.h"

#include "grid.h"
#include "line.h"
#include "picross_stats_internal.h"
#include "solver_policy.h"

#include <picross/picross.h>

#include <stdutils/macros.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <exception>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>


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
    const InputGrid::Constraints& input = get_constraints(grid, type);
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

template <typename RET_UINT>
RET_UINT progress_bar(const std::pair<float, float>& progress_bar, LineAlternatives::NbAlt progress, LineAlternatives::NbAlt nb_alternatives)
{
    static_assert(std::is_integral_v<RET_UINT> && std::is_unsigned_v<RET_UINT>);
    static_assert(std::numeric_limits<RET_UINT>::digits >= std::numeric_limits<std::uint32_t>::digits);
    assert(progress <= nb_alternatives);
    const float ratio_f = static_cast<float>(progress) / static_cast<float>(nb_alternatives);
    assert(0.f <= ratio_f && ratio_f <= 1.f);
    const float progress_f = progress_bar.first + (progress_bar.second - progress_bar.first) * ratio_f;
    const std::uint32_t ratio = reinterpret_cast<const std::uint32_t&>(progress_f);
    return static_cast<RET_UINT>(ratio);
}

std::pair<float, float> nested_progress_bar(const std::pair<float, float>& progress_bar, LineAlternatives::NbAlt progress, LineAlternatives::NbAlt nb_alternatives)
{
    assert(progress < nb_alternatives);
    const float ratio_min_f = static_cast<float>(progress)     / static_cast<float>(nb_alternatives);
    const float ratio_max_f = static_cast<float>(progress + 1) / static_cast<float>(nb_alternatives);
    assert(0.f <= ratio_min_f && ratio_min_f <= 1.f);
    assert(0.f <= ratio_max_f && ratio_max_f <= 1.f);
    return std::make_pair(progress_bar.first + (progress_bar.second - progress_bar.first) * ratio_min_f, progress_bar.first + (progress_bar.second - progress_bar.first) * ratio_max_f);
}

unsigned int compute_max_nb_of_segments(const InputGrid& input_grid)
{
    const auto max_k = [](const InputGrid::Constraints& constraints) -> std::size_t {
        return std::max_element(constraints.cbegin(), constraints.cend(), [](const InputGrid::Constraint& lhs, const InputGrid::Constraint& rhs) {
            return lhs.size() < rhs.size(); })->size();
        };
    return static_cast<unsigned int>(std::max(max_k(input_grid.rows()), max_k(input_grid.cols())));
}

}  // namespace


std::ostream& operator<<(std::ostream& out, WorkGridState state)
{
    switch(state)
    {
    case WorkGridState::INITIAL_PASS:
        out << "INITIAL_PASS";
        break;

    case WorkGridState::LINEAR_REDUCTION:
        out << "LINEAR_REDUCTION";
        break;

    case WorkGridState::FULL_REDUCTION:
        out << "FULL_REDUCTION";
        break;

    case WorkGridState::PROBING:
        out << "PROBING";
        break;

    case WorkGridState::BRANCHING:
        out << "BRANCHING";
        break;

    case WorkGridState::STOP_SOLVER:
        out << "STOP_SOLVER";
        break;

    default:
        assert(0);
        out << "UNKNOWN";
    }
    return out;
}


template <typename SolverPolicy>
WorkGrid<SolverPolicy>::WorkGrid(const InputGrid& grid, const SolverPolicy& solver_policy, Observer observer, Solver::Abort abort_function, float min_progress, float max_progress)
    : Grid(grid.width(), grid.height(), Tile::UNKNOWN, grid.name())
    , m_state(WorkGridState::INITIAL_PASS)
    , m_solver_policy(solver_policy)
    , m_max_k(compute_max_nb_of_segments(grid))
    , m_constraints()
    , m_alternatives()
    , m_line_completed()
    , m_line_has_updates()
    , m_line_is_fully_reduced()
    , m_line_probed()
    , m_nb_alternatives()
    , m_uncompleted_lines_range()
    , m_all_lines()
    , m_uncompleted_lines_end(m_all_lines.end())
    , m_grid_stats(nullptr)
    , m_observer(std::move(observer))
    , m_abort_function(std::move(abort_function))
    , m_max_nb_alternatives(SolverPolicy::MIN_NB_ALTERNATIVES)
    , m_branching_depth(0u)
    , m_probing_depth_incr(0u)
    , m_progress_bar(min_progress, max_progress)
    , m_nested_work_grid()
    , m_branch_line_cache()
    , m_full_reduction_buffers()
    , m_binomial(std::make_shared<BinomialCoefficients::Cache>())
{
    assert(m_binomial);

    if constexpr (SolverPolicy::LINE_CACHE_ENABLED)
    {
        m_branch_line_cache = LineCache(grid.width(), grid.height());
    }

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
    m_line_probed[Line::ROW].resize(height(), false);
    m_line_probed[Line::COL].resize(width(), false);
    m_nb_alternatives[Line::ROW].resize(height(), 0u);
    m_nb_alternatives[Line::COL].resize(width(), 0u);
    m_uncompleted_lines_range[Line::ROW] = { 0u, static_cast<Line::Index>(height()) };
    m_uncompleted_lines_range[Line::COL] = { 0u, static_cast<Line::Index>(width()) };

    const auto max_line_length = static_cast<unsigned int>( std::max(width(), height()));
    m_full_reduction_buffers = std::make_shared<FullReductionBuffers>(m_max_k, max_line_length);

    assert(m_constraints[Line::ROW].size() == height());
    assert(m_constraints[Line::COL].size() == width());
}

// Allocator for the nested work grid
template <typename SolverPolicy>
WorkGrid<SolverPolicy>::WorkGrid(const WorkGrid& parent)
    : Grid(parent.width(), parent.height(), Tile::UNKNOWN, parent.name())
    , m_state(WorkGridState::INITIAL_PASS)
    , m_solver_policy(parent.m_solver_policy)
    , m_max_k(parent.m_max_k)
    , m_constraints()
    , m_alternatives()
    , m_line_completed()
    , m_line_has_updates()
    , m_line_is_fully_reduced()
    , m_line_probed()
    , m_nb_alternatives()
    , m_uncompleted_lines_range()
    , m_all_lines()
    , m_uncompleted_lines_end(m_all_lines.end())
    , m_grid_stats(nullptr)
    , m_observer(parent.m_observer)
    , m_abort_function(parent.m_abort_function)
    , m_max_nb_alternatives(SolverPolicy::MIN_NB_ALTERNATIVES)
    , m_branching_depth(parent.m_branching_depth + 1u)
    , m_probing_depth_incr(0u)
    , m_progress_bar(parent.m_progress_bar)
    , m_nested_work_grid()
    , m_branch_line_cache()
    , m_full_reduction_buffers(parent.m_full_reduction_buffers)
    , m_binomial(parent.m_binomial)
{
    assert(m_binomial);

    if constexpr (SolverPolicy::LINE_CACHE_ENABLED)
    {
        m_branch_line_cache = LineCache(parent.width(), parent.height());
    }

    m_constraints[Line::ROW] = parent.m_constraints[Line::ROW];
    m_constraints[Line::COL] = parent.m_constraints[Line::COL];
    m_alternatives[Line::ROW] = build_line_alternatives_from(Line::ROW, m_constraints[Line::ROW], static_cast<const Grid&>(*this), *m_binomial);
    m_alternatives[Line::COL] = build_line_alternatives_from(Line::COL, m_constraints[Line::COL], static_cast<const Grid&>(*this), *m_binomial);
}

template <typename SolverPolicy>
WorkGrid<SolverPolicy>& WorkGrid<SolverPolicy>::operator=(const WorkGrid& parent)
{
    static_cast<Grid&>(*this) = static_cast<const Grid&>(parent);
    std::for_each(m_alternatives[Line::ROW].begin(), m_alternatives[Line::ROW].end(), [](LineAlternatives& alt) { alt.reset(); });
    std::for_each(m_alternatives[Line::COL].begin(), m_alternatives[Line::COL].end(), [](LineAlternatives& alt) { alt.reset(); });
    m_line_completed[Line::ROW] = parent.m_line_completed[Line::ROW];
    m_line_completed[Line::COL] = parent.m_line_completed[Line::COL];
    m_line_has_updates[Line::ROW] = parent.m_line_has_updates[Line::ROW];
    m_line_has_updates[Line::COL] = parent.m_line_has_updates[Line::COL];
    m_line_is_fully_reduced[Line::ROW] = parent.m_line_is_fully_reduced[Line::ROW];
    m_line_is_fully_reduced[Line::COL] = parent.m_line_is_fully_reduced[Line::COL];
    m_line_probed[Line::ROW] = parent.m_line_probed[Line::ROW];
    m_line_probed[Line::COL] = parent.m_line_probed[Line::COL];
    m_nb_alternatives[Line::ROW] = parent.m_nb_alternatives[Line::ROW];
    m_nb_alternatives[Line::COL] = parent.m_nb_alternatives[Line::COL];
    m_uncompleted_lines_range[Line::ROW] = parent.m_uncompleted_lines_range[Line::ROW];
    m_uncompleted_lines_range[Line::COL] = parent.m_uncompleted_lines_range[Line::COL];
    m_all_lines = AllLines(parent.m_all_lines.cbegin(), AllLines::const_iterator(parent.m_uncompleted_lines_end));
    m_uncompleted_lines_end = m_all_lines.end();
    m_max_nb_alternatives = SolverPolicy::MIN_NB_ALTERNATIVES;
    m_probing_depth_incr = 0u;
    return *this;
}

template <typename SolverPolicy>
WorkGrid<SolverPolicy>& WorkGrid<SolverPolicy>::nested_work_grid()
{
    if (!m_nested_work_grid)
        m_nested_work_grid = std::unique_ptr<WorkGrid<SolverPolicy>>(new WorkGrid<SolverPolicy>(*this));    // Cannot std::make_unique because the ctor is private
    assert(m_nested_work_grid);
    return *m_nested_work_grid;
}

template <typename SolverPolicy>
void WorkGrid<SolverPolicy>::configure(const SolverPolicy& solver_policy, WorkGridState initial_state, GridStats* stats, float min_progress, float max_progress)
{
    m_solver_policy = solver_policy;
    m_state = initial_state;
    set_stats(stats);
    m_progress_bar = { min_progress, max_progress };
}

template <typename SolverPolicy>
void WorkGrid<SolverPolicy>::set_stats(GridStats* stats)
{
    this->m_grid_stats = stats;
    if (stats)
    {
        stats->max_k = m_max_k;
        stats->max_branching_depth = m_branching_depth;
    }
}

template <typename SolverPolicy>
Solver::Status WorkGrid<SolverPolicy>::line_solve(const Solver::SolutionFound& solution_found)
{
    return line_solve(solution_found, false);
}

template <typename SolverPolicy>
Solver::Status WorkGrid<SolverPolicy>::line_solve(const Solver::SolutionFound& solution_found, bool currently_probing)
{
    Solver::Status status = Solver::Status::OK;
    try
    {
        bool grid_completed = false;
        PassStatus pass_status;

        while (m_state != WorkGridState::BRANCHING && m_state != WorkGridState::STOP_SOLVER && !grid_completed)
        {
            if (m_observer)
            {
                m_observer(ObserverEvent::INTERNAL_STATE, nullptr, m_branching_depth, static_cast<unsigned int>(m_state));
            }

            switch (m_state)
            {
            case WorkGridState::INITIAL_PASS:
                pass_status = full_grid_pass<WorkGridState::INITIAL_PASS>();
                m_state = WorkGridState::LINEAR_REDUCTION;
                break;

            case WorkGridState::LINEAR_REDUCTION:
                pass_status = full_grid_pass<WorkGridState::LINEAR_REDUCTION>();
                if (!pass_status.grid_changed)
                    m_state = WorkGridState::FULL_REDUCTION;
                break;

            case WorkGridState::FULL_REDUCTION:
                pass_status = full_grid_pass<WorkGridState::FULL_REDUCTION>();
                if (m_solver_policy.continue_line_solving(m_max_nb_alternatives, pass_status.grid_changed, pass_status.skipped_lines))
                {
                    // Max number of alternatives for the next full grid pass
                    m_max_nb_alternatives = m_solver_policy.get_max_nb_alternatives(m_max_nb_alternatives, pass_status.grid_changed, pass_status.skipped_lines);
                    if (pass_status.grid_changed)
                        m_state = WorkGridState::LINEAR_REDUCTION;
                }
                else if (!currently_probing && m_solver_policy.switch_to_probing(m_branching_depth, m_max_nb_alternatives, pass_status.grid_changed, pass_status.skipped_lines))
                {
                    m_state = WorkGridState::PROBING;
                }
                else if (m_solver_policy.switch_to_branching(m_max_nb_alternatives, pass_status.grid_changed, pass_status.skipped_lines))
                {
                    m_state = WorkGridState::BRANCHING;
                }
                else
                {
                    m_state = WorkGridState::STOP_SOLVER;
                }
                break;

            case WorkGridState::PROBING:
            {
                const auto probing_result = probe();
                if (probing_result.m_status == Solver::Status::CONTRADICTORY_GRID)
                    pass_status.contradictory = true;
                if (probing_result.m_grid_has_changed)
                    m_state = WorkGridState::LINEAR_REDUCTION;
                else if (!probing_result.m_continue_probing)
                    m_state = WorkGridState::BRANCHING;
                break;
            }

            case WorkGridState::BRANCHING:
            case WorkGridState::STOP_SOLVER:
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
            if (currently_probing)
            {
                status = Solver::Status::OK;
            }
            else
            {
                const bool cont = found_solution(solution_found);
                status = cont ? Solver::Status::OK : Solver::Status::ABORTED;
            }
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
        if (m_state == WorkGridState::BRANCHING)
        {
            assert(m_solver_policy.m_branching_allowed);
            if (m_observer)
            {
                m_observer(ObserverEvent::INTERNAL_STATE, nullptr, m_branching_depth, static_cast<unsigned int>(m_state));
            }

            // Make a guess (branch search)
            status = branch(solution_found);
        }
        else
        {
            assert(!m_solver_policy.m_branching_allowed);
            assert(m_branching_depth == 0);

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
bool WorkGrid<SolverPolicy>::update_line(const LineSpan& line, unsigned int nb_alt)
{
    assert(nb_alt > 0);
    static const Line DEFAULT_LINE(Line::ROW, 0, 0);
    const auto line_type = line.type();
    const auto line_index = line.index();
    const auto line_sz = line.size();
    const Line observer_original_line = m_observer ? line_from_line_span(get_line(line_type, line_index)) : DEFAULT_LINE;
    const auto observer_original_nb_alt = m_nb_alternatives[line_type][line_index];
    assert(line.size() == static_cast<unsigned int>(line.type() == Line::ROW ? width() : height()));

    bool line_changed = false;
    const auto set_tile_func = [this, &line_changed](Line::Type type, Line::Index idx) {
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
            const bool tile_changed = update(tile_idx, line_index, line[static_cast<int>(tile_idx)]);
            line_is_complete &= (grid_line[static_cast<int>(tile_idx)] != Tile::UNKNOWN);
            if (tile_changed)
                set_tile_func(Line::COL, tile_idx);
        }
    }
    else
    {
        assert(line_type == Line::COL);
        for (Line::Index tile_idx = 0u; tile_idx < line_sz; tile_idx++)
        {
            const bool tile_changed = update(line_index, tile_idx, line[static_cast<int>(tile_idx)]);
            line_is_complete &= (grid_line[static_cast<int>(tile_idx)] != Tile::UNKNOWN);
            if (tile_changed)
                set_tile_func(Line::ROW, tile_idx);
        }
    }

    m_nb_alternatives[line_type][line_index] = nb_alt = line_is_complete ? 1u : nb_alt;
    m_line_completed[line_type][line_index] = line_is_complete;
    m_line_has_updates[line_type][line_index] = line_changed;

    if (m_observer)
    {
        if (line_changed)
        {
            m_observer(ObserverEvent::KNOWN_LINE, &observer_original_line, m_branching_depth, observer_original_nb_alt);
            const Line delta = get_line(line_type, line_index) - observer_original_line;
            m_observer(ObserverEvent::DELTA_LINE, &delta, m_branching_depth, nb_alt);
        }
        else
        {
            m_observer(ObserverEvent::KNOWN_LINE, &observer_original_line, m_branching_depth, nb_alt);
        }
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
std::vector<LineId> WorkGrid<SolverPolicy>::sorted_edges() const
{
    std::vector<LineId> lines;
    for (const auto line_type : { Line::ROW, Line::COL })
    {
        if (!m_uncompleted_lines_range[line_type].empty())
        {
            const auto first = m_uncompleted_lines_range[line_type].first();
            const auto last = m_uncompleted_lines_range[line_type].last();

            lines.emplace_back(line_type, first);
            if (last != first)
                lines.emplace_back(line_type, last);
        }
    }

    std::sort(lines.begin(), lines.end(), [this](const auto& lhs, const auto& rhs) {
        return m_nb_alternatives[lhs.m_type][lhs.m_index] < m_nb_alternatives[rhs.m_type][rhs.m_index];
    });

    return lines;
}

template <typename SolverPolicy>
std::vector<LineId> WorkGrid<SolverPolicy>::sorted_lines_next_to_completed() const
{
    std::vector<LineId> lines;
    for (const auto line_type : { Line::ROW, Line::COL })
    {
        if (!m_uncompleted_lines_range[line_type].empty())
        {
            const auto first = m_uncompleted_lines_range[line_type].first();
            const auto last = m_uncompleted_lines_range[line_type].last();

            lines.emplace_back(line_type, first);
            if (last != first)
                lines.emplace_back(line_type, last);
            for (Line::Index idx = first + 1; idx < last; idx++)
            {
                if (!m_line_completed[line_type][idx] &&
                    (m_line_completed[line_type][idx - 1] || m_line_completed[line_type][idx + 1]))
                {
                    lines.emplace_back(LineId(line_type, idx));
                }
            }
        }
    }

    std::sort(lines.begin(), lines.end(), [this](const auto& lhs, const auto& rhs) {
        return m_nb_alternatives[lhs.m_type][lhs.m_index] < m_nb_alternatives[rhs.m_type][rhs.m_index];
    });

    return lines;
}


template <typename SolverPolicy>
void WorkGrid<SolverPolicy>::sort_by_nb_alternatives()
{
    std::sort(m_all_lines.begin(), m_uncompleted_lines_end, [this](const auto& lhs, const auto& rhs) {
        return m_nb_alternatives[lhs.m_type][lhs.m_index] < m_nb_alternatives[rhs.m_type][rhs.m_index];
    });
}


template <typename SolverPolicy>
bool WorkGrid<SolverPolicy>::is_sorted_by_nb_alternatives() const
{
    return std::is_sorted(m_all_lines.cbegin(), AllLines::const_iterator(m_uncompleted_lines_end), [this](const auto& lhs, const auto& rhs) {
        return m_nb_alternatives[lhs.m_type][lhs.m_index] < m_nb_alternatives[rhs.m_type][rhs.m_index];
    });
}


template <typename SolverPolicy>
LineId WorkGrid<SolverPolicy>::next_line_for_search() const
{
    assert(is_sorted_by_nb_alternatives());
    return m_all_lines.front();
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
    if (!are_compatible(get_line(type, index), reduced_line))
    {
        status.contradictory = true;
        return status;
    }

    // Set line
    status.grid_changed = update_line(reduced_line, nb_alt);

    return status;
}


template <typename SolverPolicy>
typename WorkGrid<SolverPolicy>::PassStatus WorkGrid<SolverPolicy>::single_line_linear_reduction(Line::Type type, unsigned int index)
{
    PassStatus status;

    if (m_line_is_fully_reduced[type][index] || !m_line_has_updates[type][index])
    {
        return status;
    }

    // Reduce all possible lines that match the data already present in the grid and the line constraint
    if (m_grid_stats != nullptr) { m_grid_stats->nb_single_line_linear_reduction++; }
    const auto linear_reduction = m_alternatives[type][index].linear_reduction();

    // If the list of alternative lines is empty, it means the grid data is contradictory
    if (linear_reduction.nb_alternatives == 0)
    {
        status.contradictory = true;
        return status;
    }
    if (m_grid_stats != nullptr) { m_grid_stats->max_nb_alternatives_linear = std::max(m_grid_stats->max_nb_alternatives_linear, linear_reduction.nb_alternatives); }

    // In any case, update the grid data with the reduced line resulting from the list of alternatives
    const auto nb_alternatives = std::min(linear_reduction.nb_alternatives, m_nb_alternatives[type][index]);
    const bool line_changed = status.grid_changed = update_line(linear_reduction.reduced_line, nb_alternatives);
    if (line_changed)
        m_line_is_fully_reduced[type][index] = false;
    if (linear_reduction.is_fully_reduced)
        m_line_is_fully_reduced[type][index] = true;

    if (m_grid_stats != nullptr && line_changed)
    {
        m_grid_stats->nb_single_line_linear_reduction_w_change++;
        m_grid_stats->max_nb_alternatives_linear_w_change = std::max(m_grid_stats->max_nb_alternatives_linear_w_change, linear_reduction.nb_alternatives);
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
    assert(m_full_reduction_buffers);
    const auto full_reduction = m_alternatives[type][index].full_reduction(m_full_reduction_buffers.get());

    // If the list of alternative lines is empty, it means the grid data is contradictory
    if (full_reduction.nb_alternatives == 0)
    {
        status.contradictory = true;
        return status;
    }
    if (m_grid_stats != nullptr) { m_grid_stats->max_nb_alternatives_full = std::max(m_grid_stats->max_nb_alternatives_full, full_reduction.nb_alternatives); }

    // In any case, update the grid data with the reduced line resulting from the list of alternatives
    status.grid_changed = update_line(full_reduction.reduced_line, full_reduction.nb_alternatives);

    assert(full_reduction.is_fully_reduced);
    m_line_is_fully_reduced[type][index] = true;

    if (m_grid_stats != nullptr && status.grid_changed)
    {
        m_grid_stats->nb_single_line_full_reduction_w_change++;
        m_grid_stats->max_nb_alternatives_full_w_change = std::max(m_grid_stats->max_nb_alternatives_full_w_change, full_reduction.nb_alternatives);
    }

    return status;
}


// Reduce all columns and all rows. Return false if no change was made on the grid.
// Return true if the grid was changed during the full pass
template <typename SolverPolicy>
template <WorkGridState S>
typename WorkGrid<SolverPolicy>::PassStatus WorkGrid<SolverPolicy>::full_grid_pass()
{
    PassStatus status;
    if (m_grid_stats != nullptr) { m_grid_stats->nb_full_grid_pass++; }

    for (auto it = m_all_lines.begin(); it != m_uncompleted_lines_end; ++it)
    {
        if (m_line_completed[it->m_type][it->m_index])
            continue;
        if constexpr (S == WorkGridState::INITIAL_PASS)
        {
            status += single_line_initial_pass(it->m_type, it->m_index);
        }
        else if constexpr (S == WorkGridState::LINEAR_REDUCTION)
        {
            status += single_line_linear_reduction(it->m_type, it->m_index);
        }
        else
        {
            static_assert(S == WorkGridState::FULL_REDUCTION);
            status += single_line_full_reduction(it->m_type, it->m_index);
        }
        if (status.contradictory)
        {
            if (m_observer)
            {
                const Line contradictory_line = line_from_line_span(get_line(*it));
                m_observer(ObserverEvent::KNOWN_LINE, &contradictory_line, m_branching_depth, 0);
            }
            break;
        }
        if (m_abort_function && m_abort_function())
            throw PicrossSolverAborted();
    }
    if constexpr (S == WorkGridState::INITIAL_PASS)
    {
        for (auto it = m_all_lines.begin(); it != m_uncompleted_lines_end; ++it)
            m_line_has_updates[it->m_type][it->m_index] = true;
    }
    partition_completed_lines();
    sort_by_nb_alternatives();
    return status;
}

// This method probes at least one line of the grid. It returns true if the grid has changed, false otherwise.
// To prevent repeat of the found solutions, if the grid is solved during the probing, the found solution is not saved.
template <typename SolverPolicy>
typename WorkGrid<SolverPolicy>::ProbingResult WorkGrid<SolverPolicy>::probe()
{
    assert(m_solver_policy.m_branching_allowed);
    ProbingResult result{};
    std::vector<LineId> candidate_lines;
    const auto edges = sorted_edges();
    for (const LineId& edge : edges)
    {
        if (m_nb_alternatives[edge.m_type][edge.m_index] < m_solver_policy.m_max_nb_alternatives_probing_edge)
        {
            candidate_lines.emplace_back(edge);
        }
    }
    assert(is_sorted_by_nb_alternatives());
    for (auto idx = 0u; idx < m_solver_policy.m_nb_of_lines_for_probing_round && idx < m_all_lines.size(); idx++)
    {
        const LineId& line_id = m_all_lines[idx];
        if (!m_line_completed[line_id.m_type][line_id.m_index] &&
            m_nb_alternatives[line_id.m_type][line_id.m_index] < m_solver_policy.m_max_nb_alternatives_probing_other)
        {
            candidate_lines.emplace_back(line_id);
        }
    }
    for (auto candidate : candidate_lines)
    {
        if (!m_line_probed[candidate.m_type][candidate.m_index])
        {
            result = probe(candidate);
            switch (result.m_status)
            {
            case Solver::Status::OK:
                break;

            case Solver::Status::ABORTED:
            case Solver::Status::CONTRADICTORY_GRID:
                return result;

            case Solver::Status::NOT_LINE_SOLVABLE:
            default:
                assert(0);
                break;
            }
            if (result.m_grid_has_changed)
            {
                result.m_continue_probing = true;
                m_probing_depth_incr = 1u;
                break;
            }
        }
    }
    return result;
}

// This method probes a specific line. It returns true if the grid was changed, false otherwise
template <typename SolverPolicy>
typename WorkGrid<SolverPolicy>::ProbingResult WorkGrid<SolverPolicy>::probe(LineId line_id)
{
    ProbingResult result{};
    assert(m_line_is_fully_reduced[line_id.m_type][line_id.m_index]);

    const LineConstraint& line_constraint = m_constraints[line_id.m_type][line_id.m_index];
    const LineSpan known_tiles = get_line(line_id.m_type, line_id.m_index);

    // Probe lines only once per solve
    assert(!m_line_probed[line_id.m_type][line_id.m_index]);
    m_line_probed[line_id.m_type][line_id.m_index] = true;

    // Cache the full reduction of all the possible orthogonal lines
    if constexpr (SolverPolicy::LINE_CACHE_ENABLED)
    {
        fill_cache_with_orthogonal_lines(line_id);
    }

    // Build all alternatives for that row or column
    const auto list_of_all_alternatives = line_constraint.build_all_possible_lines(known_tiles);
    assert(!list_of_all_alternatives.empty());  // Then the grid would be contradictory, but this must be catched earlier
    const auto nb_alt = static_cast<unsigned int>(list_of_all_alternatives.size());
    assert(nb_alt >= 2);

    if (m_observer)
    {
        const auto line_known_tiles = line_from_line_span(known_tiles);
        m_observer(ObserverEvent::BRANCHING, &line_known_tiles, m_branching_depth, nb_alt);
    }

    if (m_grid_stats != nullptr)
    {
        m_grid_stats->nb_probing_calls++;
        m_grid_stats->total_nb_probing_alternatives += nb_alt;
    }

    std::optional<GridSnapshot<Line::ROW>> reduced_grid;
    Solver::SolutionFound do_nothing_with_found_solution = [](Solver::Solution&&) -> bool { return true; };
    auto nested_solver_policy = m_solver_policy;
    nested_solver_policy.m_branching_allowed = false;
    LineAlternatives::NbAlt progress = 0u;
    auto& probing_work_grid = nested_work_grid();
    for (const Line& guess_line : list_of_all_alternatives)
    {
        // Copy current grid state to a nested grid
        const auto nested_progress = nested_progress_bar(m_progress_bar, progress, nb_alt);
        std::unique_ptr<GridStats> nested_stats = m_grid_stats ? std::make_unique<GridStats>() : nullptr;
        probing_work_grid = *this;
        probing_work_grid.configure(nested_solver_policy, WorkGridState::LINEAR_REDUCTION, nested_stats.get(), nested_progress.first, nested_progress.second);
        if (m_observer)
        {
            m_observer(ObserverEvent::BRANCHING, nullptr, probing_work_grid.m_branching_depth, 0);
        }

        // Set one line in the new_grid according to the hypothesis we made. That line is then complete
        probing_work_grid.update_line(guess_line, 1u);
        probing_work_grid.m_line_is_fully_reduced[guess_line.type()][guess_line.index()] = true;
        assert(probing_work_grid.m_line_completed[guess_line.type()][guess_line.index()]);

        // Set orthogonal lines retrived from cache
        if constexpr (SolverPolicy::LINE_CACHE_ENABLED)
        {
            set_orthogonal_lines_from_cache(probing_work_grid, guess_line);
        }

        probing_work_grid.partition_completed_lines();

        // Solve the new grid!
        const auto status = probing_work_grid.line_solve(do_nothing_with_found_solution, true);

        if (m_grid_stats)
        {
            assert(nested_stats);
            merge_branching_grid_stats(*m_grid_stats, *nested_stats);
        }

        if (status == Solver::Status::ABORTED)
        {
            result.m_status = status;
            return result;
        }

        if (status != Solver::Status::CONTRADICTORY_GRID)
        {
            if (!reduced_grid)
                reduced_grid.emplace(static_cast<Grid&>(probing_work_grid));
            else
                reduced_grid->reduce(static_cast<Grid&>(probing_work_grid));
        }

        progress++;
    }
#ifndef NDEBUG
    m_branch_line_cache.clear();
#endif

    // Repeat start branching message
    if (m_observer)
    {
        const auto line_known_tiles = line_from_line_span(known_tiles);
        m_observer(ObserverEvent::BRANCHING, &line_known_tiles, m_branching_depth, nb_alt);
    }

    if (!reduced_grid)
    {
        result.m_status = Solver::Status::CONTRADICTORY_GRID;
        return result;
    }

    for (Line::Index row_idx = 0; row_idx < height(); row_idx++)
    {
        const auto reduced_line = reduced_grid->get_line(row_idx);
        const auto nb_alternatives = m_nb_alternatives[Line::ROW][row_idx];
        const bool line_changed = update_line(reduced_line, nb_alternatives);
        result.m_grid_has_changed |= line_changed;
        if (line_changed)
            m_line_is_fully_reduced[Line::ROW][row_idx] = false;
    }

    return result;
}

// This method will test a range of alternatives for one particular line of the grid, each time
// creating a new instance of the grid class on which the function WorkGrid<SolverPolicy>::solve() is called.
template <typename SolverPolicy>
Solver::Status WorkGrid<SolverPolicy>::branch(const Solver::SolutionFound& solution_found)
{
    assert(m_solver_policy.m_branching_allowed);
    assert(m_all_lines.begin() != m_uncompleted_lines_end);

    const LineId search_line = next_line_for_search();
    assert(m_line_is_fully_reduced[search_line.m_type][search_line.m_index]);
    const LineConstraint& line_constraint = m_constraints[search_line.m_type][search_line.m_index];
    const LineSpan known_tiles = get_line(search_line.m_type, search_line.m_index);
    const auto nb_alt = m_nb_alternatives[search_line.m_type][search_line.m_index];
    assert(nb_alt >= 2);

    // Cache the full reduction of all the possible orthogonal lines
    if constexpr (SolverPolicy::LINE_CACHE_ENABLED)
    {
        fill_cache_with_orthogonal_lines(search_line);
    }

    // Build all alternatives for that row or column
    const auto list_of_all_alternatives = line_constraint.build_all_possible_lines(known_tiles);
    assert(list_of_all_alternatives.size() == nb_alt);
    assert(!list_of_all_alternatives.empty());  // Then the grid would be contradictory, but this must be catched earlier

    if (m_observer)
    {
        const auto line_known_tiles = line_from_line_span(known_tiles);
        m_observer(ObserverEvent::BRANCHING, &line_known_tiles, m_branching_depth, nb_alt);
    }

    if (m_grid_stats != nullptr)
    {
        m_grid_stats->nb_branching_calls++;
        m_grid_stats->total_nb_branching_alternatives += nb_alt;
        if (m_grid_stats->max_nb_alternatives_by_branching_depth.size() < m_branching_depth + 1)
            m_grid_stats->max_nb_alternatives_by_branching_depth.resize(m_branching_depth + 1, 0u);
        auto& max_nb_alt = m_grid_stats->max_nb_alternatives_by_branching_depth[m_branching_depth];
        max_nb_alt = std::max(max_nb_alt, nb_alt);
    }

    auto nested_solver_policy = m_solver_policy;

    Solver::Status status = Solver::Status::OK;
    bool flag_solution_found = false;
    LineAlternatives::NbAlt progress = 0u;
    auto& branching_work_grid = nested_work_grid();
    for (const Line& guess_line : list_of_all_alternatives)
    {
        // Copy current grid state to a nested grid
        const auto nested_progress = nested_progress_bar(m_progress_bar, progress, nb_alt);
        std::unique_ptr<GridStats> nested_stats = m_grid_stats ? std::make_unique<GridStats>() : nullptr;
        branching_work_grid = *this;
        branching_work_grid.configure(nested_solver_policy, WorkGridState::LINEAR_REDUCTION, nested_stats.get(), nested_progress.first, nested_progress.second);
        if (m_observer)
        {
            m_observer(ObserverEvent::BRANCHING, nullptr, branching_work_grid.m_branching_depth, 0);
        }

        // Set one line in the new_grid according to the hypothesis we made. That line is then complete
        branching_work_grid.update_line(guess_line, 1u);
        branching_work_grid.m_line_is_fully_reduced[guess_line.type()][guess_line.index()] = true;
        assert(branching_work_grid.m_line_completed[guess_line.type()][guess_line.index()]);

        // Set orthogonal lines retrived from cache
        if constexpr (SolverPolicy::LINE_CACHE_ENABLED)
        {
            set_orthogonal_lines_from_cache(branching_work_grid, guess_line);
        }

        branching_work_grid.partition_completed_lines();

        // Solve the new grid!
        status = branching_work_grid.solve(solution_found);

        if (m_grid_stats)
        {
            assert(nested_stats);
            merge_branching_grid_stats(*m_grid_stats, *nested_stats);
        }

        flag_solution_found |= (status == Solver::Status::OK);

        // Progress bar
        progress++;
        if (m_observer)
        {
            m_observer(ObserverEvent::PROGRESS, nullptr, branching_work_grid.m_branching_depth, progress_bar<unsigned int>(m_progress_bar, progress, nb_alt));
        }

        if (status == Solver::Status::ABORTED)
            return status;
    }
#ifndef NDEBUG
    m_branch_line_cache.clear();
#endif

    // Repeat start branching message
    if (m_observer)
    {
        const auto line_known_tiles = line_from_line_span(known_tiles);
        m_observer(ObserverEvent::BRANCHING, &line_known_tiles, m_branching_depth, nb_alt);
    }

    return flag_solution_found ? Solver::Status::OK : Solver::Status::CONTRADICTORY_GRID;
}


template <typename SolverPolicy>
bool WorkGrid<SolverPolicy>::is_valid_solution() const
{
    assert(is_completed());
    bool valid = true;
    for (unsigned int x = 0u; x < width(); x++)  { valid &= m_constraints[Line::COL][x].compatible(get_line(Line::COL, x)); }
    for (unsigned int y = 0u; y < height(); y++) { valid &= m_constraints[Line::ROW][y].compatible(get_line(Line::ROW, y)); }
    return valid;
}


template <typename SolverPolicy>
bool WorkGrid<SolverPolicy>::found_solution(const Solver::SolutionFound& solution_found) const
{
    assert(is_valid_solution());
    const auto adjusted_branching_depth = m_branching_depth + m_probing_depth_incr;
    if (m_grid_stats != nullptr) { m_grid_stats->nb_solutions++; }
    if (m_observer) { m_observer(ObserverEvent::SOLVED_GRID, nullptr, adjusted_branching_depth, 0); }

    // Shallow copy of only the grid data
    return solution_found(Solver::Solution{ OutputGrid(*this), adjusted_branching_depth, FULL_SOLUTION });
}

template <typename SolverPolicy>
void WorkGrid<SolverPolicy>::fill_cache_with_orthogonal_lines(LineId line_id)
{
    assert(SolverPolicy::LINE_CACHE_ENABLED);
    const LineSpan known_tiles = get_line(line_id);
    const Line::Type orth_type = line_id.m_type == Line::ROW ? Line::COL : Line::ROW;
    std::size_t orth_idx = 0u;
    for (Tile tile : known_tiles)
    {
        if (tile == Tile::UNKNOWN)
        {
            const LineId orth_line_id(orth_type, orth_idx);
            const LineConstraint& constraint = m_constraints[orth_type][orth_idx];
            Line orth_line = line_from_line_span(get_line(orth_line_id));
            assert(orth_line[line_id.m_index] == Tile::UNKNOWN);
            for (Tile key : { Tile::EMPTY, Tile::FILLED })
            {
                orth_line[line_id.m_index] = key;
                assert(m_full_reduction_buffers);
                const auto reduction = LineAlternatives(constraint, orth_line, *m_binomial).full_reduction(m_full_reduction_buffers.get());
                m_branch_line_cache.store_line(orth_line_id, key, reduction.reduced_line, reduction.nb_alternatives);
                if (m_grid_stats != nullptr)
                {
                    m_grid_stats->nb_single_line_full_reduction++;
                    m_grid_stats->max_nb_alternatives_full = std::max(m_grid_stats->max_nb_alternatives_full, reduction.nb_alternatives);
                    // Ignore w_change stats here
                }
            }
        }
        orth_idx++;
    }
}

template <typename SolverPolicy>
void WorkGrid<SolverPolicy>::set_orthogonal_lines_from_cache(WorkGrid& target_grid, const LineSpan& alternative) const
{
    assert(SolverPolicy::LINE_CACHE_ENABLED);
    const LineId line_id(alternative.type(), alternative.index());
    const LineSpan known_tiles = get_line(line_id);
    const Line::Type orth_type = line_id.m_type == Line::ROW ? Line::COL : Line::ROW;
    std::size_t orth_idx = 0u;
    for (Tile tile : known_tiles)
    {
        if (tile == Tile::UNKNOWN)
        {
            const LineId orth_line_id(orth_type, orth_idx);
            const Tile key = alternative[static_cast<int>(orth_idx)];
            assert(key != Tile::UNKNOWN);
            const auto orth_line_entry = m_branch_line_cache.read_line(orth_line_id, key);
            target_grid.update_line(orth_line_entry.m_line_span, orth_line_entry.m_nb_alt);

            // Since all lines of the grid are supposed to have been fully reduced before the solver will probe/branch, we are not supposed
            // to be in the situation where nb_alt = 0 (which would mean the tile on the orthogonal line could be found by a line solve).
            // If that still happens in Release, the line is not set as fully reduced therefore the contradicton will be detected later on.
            assert(orth_line_entry.m_nb_alt != 0);
            assert(orth_line_entry.m_nb_alt != 1 || target_grid.m_line_completed[orth_type][orth_idx]);
            target_grid.m_line_is_fully_reduced[orth_type][orth_idx] = (orth_line_entry.m_nb_alt > 0);
        }
        orth_idx++;
    }
}

// Explicit template instantiations
template class WorkGrid<SolverPolicy_RampUpMaxNbAlternatives>;

} // namespace picross
