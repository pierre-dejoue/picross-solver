/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PRIVATE API of the Picross solver
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include <picross/picross.h>


namespace picross
{

/*
 * Grid Solver: an implementation
 */
class RefSolver : public Solver
{
public:
    Solver::Solutions solve(const InputGrid& grid_input, unsigned int max_nb_solutions) const override;
    void set_observer(Observer observer) override;
    void set_stats(GridStats& stats) override;
private:
    Observer observer;
    GridStats* stats;
};

} // namespace picross
