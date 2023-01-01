#pragma once

namespace picross
{

struct SolverPolicyBase
{
    bool m_branching_allowed = false;
    bool m_limit_on_max_nb_alternatives = false;
    unsigned int m_min_nb_alternatives = 1 << 6;
    unsigned int m_max_nb_alternatives = 1 << 30;
};

struct SolverPolicy_RampUpMaxNbAlternatives : public SolverPolicyBase
{
    unsigned int get_max_nb_alternatives(unsigned int previous_max_nb_alternatives, bool grid_changed, unsigned int skipped_lines) const;
    bool switch_to_branching(unsigned int max_nb_alternatives, bool grid_changed, unsigned int skipped_lines) const;
};

} // namespace picross
