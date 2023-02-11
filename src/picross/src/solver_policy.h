#pragma once

namespace picross
{

struct SolverPolicyBase
{
    static constexpr unsigned int MIN_NB_ALTERNATIVES = 1 << 10;
    static constexpr unsigned int PARTIAL_REDUCE_NB_CONSTRAINTS = 1;

    bool m_branching_allowed = false;
    bool m_limit_on_max_nb_alternatives = false;
    unsigned int m_min_nb_alternatives_for_linear_reduction = 1 << 8;
    unsigned int m_max_nb_alternatives_for_probing = 1 << 12;
    unsigned int m_max_nb_alternatives_while_probing = 1 << 16;
    unsigned int m_max_nb_alternatives = 1 << 20;
};

struct SolverPolicy_RampUpMaxNbAlternatives : public SolverPolicyBase
{
    unsigned int get_max_nb_alternatives(unsigned int previous_max_nb_alternatives, bool grid_changed, unsigned int skipped_lines) const;
    bool switch_to_branching(unsigned int max_nb_alternatives, bool grid_changed, unsigned int skipped_lines) const;
};

} // namespace picross
