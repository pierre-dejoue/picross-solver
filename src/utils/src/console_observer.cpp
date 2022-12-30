#include <utils/console_observer.h>

#include <cassert>
#include <exception>


ConsoleObserver::ConsoleObserver(size_t width, size_t height, std::ostream& ostream)
    : GridObserver(width, height)
    , ostream(ostream)
    , nb_solved_grids(0u)
{
}

void ConsoleObserver::observer_callback(picross::Solver::Event event, const picross::Line* delta, unsigned int depth, unsigned int misc, const ObserverGrid& grid)
{
    ostream << event;
    switch (event)
    {
    case picross::Solver::Event::BRANCHING:
        if (delta)
        {
            ostream << " NODE";
            ostream << " known_tiles: " << str_line_full(*delta);
            ostream << " depth: " << depth;
            ostream << " nb_alt: " << misc;
        }
        else
        {
            ostream << " EDGE";
            ostream << " depth: " << depth;
        }
        break;

    case picross::Solver::Event::DELTA_LINE:
        ostream << " delta: " << str_line_full(*delta)
                << " depth: " << depth
                << " nb_alt: " << misc;
        break;

    case picross::Solver::Event::SOLVED_GRID:
        ostream << " nb " << ++nb_solved_grids << std::endl;
        ostream << grid;
        break;

    case picross::Solver::Event::INTERNAL_STATE:
        ostream << " state: " << misc;
        break;

    default:
        assert(0);  // Unknown Solver::Event
    }
    ostream << std::endl;
}
