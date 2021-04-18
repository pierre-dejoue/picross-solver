#include <grid_observer.h>

#include <cassert>
#include <exception>


GridObserver::GridObserver(size_t width, size_t height)
    : grids()
    , current_depth(0u)
{
    grids.emplace_back(width, height);
}

void GridObserver::operator()(picross::Solver::Event event, const picross::Line* delta, unsigned int depth)
{
    const auto width = grids[0].get_width();
    const auto height = grids[0].get_height();

    switch (event)
    {
    case picross::Solver::Event::BRANCHING:
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
        assert(depth == current_depth);
        const size_t index = delta->get_index();
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
        break;

    default:
        throw std::invalid_argument("Unknown Solver::Event");
    }

    assert(current_depth < grids.size());
    observer_callback(event, delta, depth, grids.at(current_depth));
}

void GridObserver::observer_clear()
{
    assert(!grids.empty());
    grids.erase(grids.begin() + 1, grids.end());
    assert(grids.size() == 1);
    grids[0].reset();
    current_depth = 0u;
}
