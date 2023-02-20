#pragma once

namespace picross
{

struct SolverPolicyBase
{
    static constexpr unsigned int MIN_NB_ALTERNATIVES = 1 << 10;
    static constexpr unsigned int PARTIAL_REDUCE_NB_CONSTRAINTS = 1;

    bool m_branching_allowed = false;
    bool m_limit_on_max_nb_alternatives = false;
    unsigned int m_nb_of_lines_for_probing_round = 12;
    unsigned int m_max_nb_alternatives_probing_edge  = 1 << 12;
    unsigned int m_max_nb_alternatives_probing_other = 1 << 8;
    unsigned int m_max_nb_alternatives = 1 << 26;
};

struct SolverPolicy_RampUpMaxNbAlternatives : public SolverPolicyBase
{
    unsigned int get_max_nb_alternatives(unsigned int previous_max_nb_alternatives, bool grid_changed, unsigned int skipped_lines) const;
    bool continue_line_solving(unsigned int max_nb_alternatives, bool grid_changed, unsigned int skipped_lines) const;
    bool switch_to_branching(unsigned int max_nb_alternatives, bool grid_changed, unsigned int skipped_lines) const;
    bool switch_to_probing(unsigned int branching_depth, unsigned int max_nb_alternatives, bool grid_changed, unsigned int skipped_lines) const;
};

} // namespace picross
