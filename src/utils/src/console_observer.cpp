#include <utils/console_observer.h>

#include <cassert>
#include <exception>


ConsoleObserver::ConsoleObserver(size_t width, size_t height, std::ostream& ostream)
    : GridObserver(width, height)
    , ostream(ostream)
    , nb_solved_grids(0u)
{
}

void ConsoleObserver::observer_callback(picross::Solver::Event event, const picross::Line* delta, unsigned int depth, const ObserverGrid& grid)
{
    ostream << event;
    switch (event)
    {
    case picross::Solver::Event::BRANCHING:
        ostream << " depth: " << depth << std::endl;
        break;

    case picross::Solver::Event::DELTA_LINE:
        ostream << " delta: " << *delta << " depth: " << depth << std::endl;
        break;

    case picross::Solver::Event::SOLVED_GRID:
        ostream << " nb " << ++nb_solved_grids << std::endl;
        ostream << grid << std::endl;
        break;

    default:
        throw std::invalid_argument("Unknown Solver::Event");
    }
}
