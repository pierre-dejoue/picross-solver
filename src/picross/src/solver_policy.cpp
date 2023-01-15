#include "solver_policy.h"

#include "macros.h"

#include <algorithm>
#include <cassert>
#include <limits>

namespace picross
{

unsigned int SolverPolicy_RampUpMaxNbAlternatives::get_max_nb_alternatives(unsigned int previous_max_nb_alternatives, bool grid_changed, unsigned int skipped_lines) const
{
    unsigned int nb_alternatives = previous_max_nb_alternatives;
    if (grid_changed && previous_max_nb_alternatives > MIN_NB_ALTERNATIVES)
    {
        // Decrease max_nb_alternatives
        nb_alternatives = std::min(nb_alternatives, m_max_nb_alternatives) >> 2;
    }
    else if (!grid_changed && skipped_lines > 0u)
    {
        // Increase max_nb_alternatives
        nb_alternatives = nb_alternatives >= m_max_nb_alternatives
            ? std::numeric_limits<unsigned int>::max()
            : nb_alternatives << 2;

        if (m_limit_on_max_nb_alternatives)
            nb_alternatives = std::min(nb_alternatives, m_max_nb_alternatives);
    }
    return nb_alternatives;
}

bool SolverPolicy_RampUpMaxNbAlternatives::switch_to_branching(unsigned int max_nb_alternatives, bool grid_changed, unsigned int skipped_lines) const
{
    return !grid_changed &&
        (skipped_lines == 0u || (m_limit_on_max_nb_alternatives && max_nb_alternatives >= m_max_nb_alternatives));
}

} // namespace picross
