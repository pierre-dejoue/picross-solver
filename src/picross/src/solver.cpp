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

std::pair<Solver::Status, Solver::Solutions> RefSolver::solve(const InputGrid& grid_input, unsigned int max_nb_solutions) const
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

    auto status = Status::OK;
    try
    {
        status = WorkGrid<LineSelectionPolicy_RampUpNbAlternatives>(grid_input, &solutions, stats, std::move(observer_wrapper), abort_function).solve(max_nb_solutions);
    }
    catch (const PicrossSolverAborted&)
    {
        status = Status::ABORTED;
    }

    return std::make_pair(status, std::move(solutions));
}


void RefSolver::set_observer(Observer observer)
{
    this->observer = std::move(observer);
}


void RefSolver::set_stats(GridStats& stats)
{
    this->stats = &stats;
}

void RefSolver::set_abort_function(Abort abort)
{
    this->abort_function = std::move(abort);
}


std::ostream& operator<<(std::ostream& ostream, Solver::Status status)
{
    switch (status)
    {
    case Solver::Status::OK:
        ostream << "OK";
        break;
    case Solver::Status::ABORTED:
        ostream << "ABORTED";
        break;
    case Solver::Status::CONTRADICTORY_GRID:
        ostream << "CONTRADICTORY_GRID";
        break;
    default:
        throw std::invalid_argument("Unknown Solver::Status");
    }
    return ostream;
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


std::pair<int, std::string> validate_input_grid(const Solver& solver, const InputGrid& grid_input)
{
    bool check;
    std::string check_msg;
    std::tie(check, check_msg) = picross::check_grid_input(grid_input);

    if (!check)
    {
        return std::make_pair(-1, check_msg);
    }

    Solver::Status status;
    Solver::Solutions solutions;
    std::tie(status, solutions) = solver.solve(grid_input, 2);

    if (status == Solver::Status::CONTRADICTORY_GRID)
    {
        return std::make_pair(-1, "Input grid constraints are contradictory");
    }

    assert(status == Solver::Status::OK);
    std::string message;
    if (solutions.empty())
    {
        message = "No solution could be found";
    }
    else if (solutions.size() > 1)
    {
        message = "The solution is not unique";
    }
    return std::make_pair(static_cast<ValidationCode>(solutions.size()), message);
}


std::string validation_code_str(ValidationCode code)
{
    if (code < 0)
    {
        return "ERR";
    }
    else if (code == 0)
    {
        return "ZERO";
    }
    else if(code == 1)
    {
        return "OK";
    }
    else
    {
        assert(code > 1);
        return "MULT";
    }
}


std::unique_ptr<Solver> get_ref_solver()
{
    return std::make_unique<RefSolver>();
}


} // namespace picross
