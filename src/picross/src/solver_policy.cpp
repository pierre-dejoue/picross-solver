#include "solver_policy.h"

#include <stdutils/macros.h>

#include <algorithm>
#include <cassert>
#include <limits>

namespace picross
{

unsigned int SolverPolicy_RampUpMaxNbAlternatives::get_max_nb_alternatives(unsigned int previous_max_nb_alternatives, bool grid_changed, unsigned int skipped_lines) const
{
    constexpr auto MAX = std::numeric_limits<unsigned int>::max();
    unsigned int nb_alternatives = previous_max_nb_alternatives;
    if (grid_changed && previous_max_nb_alternatives > MIN_NB_ALTERNATIVES)
    {
        // Decrease max_nb_alternatives
        nb_alternatives = std::min(previous_max_nb_alternatives, m_max_nb_alternatives) >> 4;
    }
    else if (!grid_changed && skipped_lines > 0u)
    {
        // Increase max_nb_alternatives
        nb_alternatives = nb_alternatives >= m_max_nb_alternatives ? MAX : (nb_alternatives << 2);
        const auto max_nb_alternatives = m_limit_on_max_nb_alternatives ? m_max_nb_alternatives : MAX;
        nb_alternatives = std::min(nb_alternatives, max_nb_alternatives);
    }
    return nb_alternatives;
}

bool SolverPolicy_RampUpMaxNbAlternatives::continue_line_solving(unsigned int max_nb_alternatives, bool grid_changed, unsigned int skipped_lines) const
{
    return grid_changed || (skipped_lines != 0u && (!m_limit_on_max_nb_alternatives || max_nb_alternatives < m_max_nb_alternatives));
}

bool SolverPolicy_RampUpMaxNbAlternatives::switch_to_branching(unsigned int max_nb_alternatives, bool grid_changed, unsigned int skipped_lines) const
{
    return m_branching_allowed && !continue_line_solving(max_nb_alternatives, grid_changed, skipped_lines);
}

bool SolverPolicy_RampUpMaxNbAlternatives::switch_to_probing(unsigned int branching_depth, unsigned int max_nb_alternatives, bool grid_changed, unsigned int skipped_lines) const
{
    return m_branching_allowed && branching_depth == 0 && !continue_line_solving(max_nb_alternatives, grid_changed, skipped_lines);
}

} // namespace picross
