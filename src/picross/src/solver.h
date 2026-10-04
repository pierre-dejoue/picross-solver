/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PRIVATE API of the Picross solver
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include <picross/picross.h>

namespace picross {

/*
 * Grid Solver: an implementation
 */
template <bool BranchingAllowed = true>
class RefSolver : public Solver
{
public:
    explicit RefSolver(SolverFlags flags = {}): m_observer(), m_stats(nullptr), m_abort_function(), m_flags(flags) {}
    Result solve(const InputGrid& input_grid, unsigned int max_nb_solutions) const override;
    Status solve(const InputGrid& input_grid, SolutionFound solution_found) const override;
    void set_observer(Observer observer) override;
    void set_stats(GridStats& stats) override;
    void set_abort_function(Abort abort) override;
private:
    Observer m_observer;
    GridStats* m_stats;
    Abort m_abort_function;
    SolverFlags m_flags;
};

} // namespace picross
