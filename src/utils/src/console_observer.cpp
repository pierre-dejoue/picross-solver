#include <console_observer.h>

#include <cassert>
#include <exception>


ConsoleObserver::ConsoleObserver(size_t width, size_t height, std::ostream& ostream)
    : GridObserver(width, height)
    , ostream(ostream)
{
}

void ConsoleObserver::callback(picross::Solver::Event event, const picross::Line* delta, unsigned int index, unsigned int depth, const picross::OutputGrid& grid)
{
    ostream << event;
    switch (event)
    {
    case picross::Solver::Event::BRANCHING:
        ostream << " depth: " << depth << std::endl;
        break;

    case picross::Solver::Event::DELTA_LINE:
        ostream << " delta: " << *delta << " index: " << index << " depth: " << depth << std::endl;
        break;

    case picross::Solver::Event::SOLVED_GRID:
        ostream << " " << grid << std::endl;
        break;

    default:
        throw std::invalid_argument("Unknown Solver::Event");
    }
}
