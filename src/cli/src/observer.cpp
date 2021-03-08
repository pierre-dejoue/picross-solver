#include "observer.h"

#include <cassert>


ConsoleObserver::ConsoleObserver(size_t width, size_t height, std::ostream& ostream) :
    ostream(ostream),
    grids(),
    current_depth(0u)
{
    grids.emplace_back(width, height);
}

void ConsoleObserver::operator()(picross::Solver::Event event, const picross::Line* delta, unsigned int index, unsigned int depth)
{
    const auto width = grids[0].get_width();
    const auto height = grids[0].get_height();

    ostream << event;
    switch (event)
    {
    case picross::Solver::Event::BRANCHING:
        ostream << " depth: " << depth << std::endl;
        if (depth > current_depth)
        {
            assert(depth == current_depth + 1u);
            for (size_t idx = grids.size(); idx <= depth; ++idx)
            {
                grids.emplace_back(width, height);
            }
        }
        assert(depth > 0u);
        assert(depth < grids.size());
        grids[depth] = grids[depth - 1];
        current_depth = depth;
        break;

    case picross::Solver::Event::DELTA_LINE:
    {
        ostream << " delta: " << *delta << " index: " << index << " depth: " << depth << std::endl;
        assert(depth == current_depth);
        picross::OutputGrid& grid = grids.at(current_depth);
        if (delta->get_type() == picross::Line::ROW)
        {
            for (size_t x = 0u; x < width; ++x)
                if (delta->at(x) != picross::Tile::UNKNOWN)
                {
                    grid.set(x, index, delta->at(x));
                }
        }
        else
        {
            for (size_t y = 0u; y < height; ++y)
                if (delta->at(y) != picross::Tile::UNKNOWN)
                {
                    grid.set(index, y, delta->at(y));
                }
        }
        break;
    }

    case picross::Solver::Event::SOLVED_GRID:
        assert(depth == current_depth);
        ostream << " " << grids.at(current_depth) << std::endl;
        break;

    default:
        throw std::invalid_argument("Unknown Solver::Event");
    }
}
