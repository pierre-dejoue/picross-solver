/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#include "solver.h"

#include <picross/picross.h>

#include "work_grid.h"

#include <cassert>
#include <exception>
#include <memory>
#include <ostream>
#include <string>
#include <utility>


namespace picross
{

Solver::Solutions RefSolver::solve(const InputGrid& grid_input, unsigned int max_nb_solutions) const
{
    Solutions solutions;

    if (stats != nullptr)
    {
        /* Reset stats */
        std::swap(*stats, GridStats());

        stats->max_nb_solutions = max_nb_solutions;
    }

    Observer observer_wrapper;
    if (observer)
    {
        observer_wrapper = [this](Solver::Event event, const Line* delta, unsigned int depth)
        {
            if (this->stats != nullptr)
            {
                this->stats->nb_observer_callback_calls++;
            }
            this->observer(event, delta, depth);
        };
    }

    const bool success = WorkGrid<LineSelectionPolicy_RampUpNbAlternatives>(grid_input, &solutions, stats, std::move(observer_wrapper)).solve(max_nb_solutions);

    assert(success == (solutions.size() > 0u));
    return solutions;
}


void RefSolver::set_observer(Observer observer)
{
    this->observer = std::move(observer);
}


void RefSolver::set_stats(GridStats& stats)
{
    this->stats = &stats;
}


std::ostream& operator<<(std::ostream& ostream, Solver::Event event)
{
    switch (event)
    {
    case Solver::Event::BRANCHING:
        ostream << "BRANCHING";
        break;
    case Solver::Event::DELTA_LINE:
        ostream << "DELTA_LINE";
        break;
    case Solver::Event::SOLVED_GRID:
        ostream << "SOLVED_GRID";
        break;
    default:
        throw std::invalid_argument("Unknown Solver::Event");
    }
    return ostream;
}


std::pair<bool, std::string> validate_input_grid(const Solver& solver, const InputGrid& grid_input)
{
    bool check;
    std::string check_msg;
    std::tie(check, check_msg) = picross::check_grid_input(grid_input);

    if (!check)
    {
        return std::make_pair(false, check_msg);
    }

    const auto solutions = solver.solve(grid_input, 2);

    if (solutions.empty())
    {
        return std::make_pair(false, "No solution could be found");
    }
    else if (solutions.size() > 1)
    {
        return std::make_pair(false, "The grid does not have a unique solution");
    }

    return std::make_pair(true, std::string());
}


std::unique_ptr<Solver> get_ref_solver()
{
    return std::make_unique<RefSolver>();
}


} // namespace picross
