#include <utils/grid_observer.h>

#include <cassert>
#include <exception>
#include <iostream>


ObserverGrid::ObserverGrid(size_t width, size_t height, const std::string& name)
    : OutputGrid(width, height, name)
    , depth_grid(height * width, 0u)
{
}

void ObserverGrid::set_tile(size_t x, size_t y, picross::Tile t, unsigned int d)
{
    OutputGrid::set_tile(x, y, t);
    depth_grid[x * height() + y] = d;
}

unsigned int ObserverGrid::get_depth(size_t x, size_t y) const
{
    return depth_grid[x * height() + y];
}


GridObserver::GridObserver(size_t width, size_t height)
    : grids()
    , current_depth(0u)
{
    grids.emplace_back(width, height);
}

GridObserver::GridObserver(const picross::InputGrid& grid)
    : GridObserver(grid.width(), grid.height())
{
}

void GridObserver::operator()(picross::Solver::Event event, const picross::Line* line, unsigned int depth, unsigned int misc)
{
    const auto width = grids[0].width();
    const auto height = grids[0].height();

    switch (event)
    {
    case picross::Solver::Event::BRANCHING:
        if (!line)
        {
            // BRANCHING EDGE event
            assert(depth > 0u);
            if (depth > current_depth)
            {
                assert(depth == current_depth + 1u);
                for (size_t idx = grids.size(); idx <= depth; ++idx)
                {
                    grids.emplace_back(width, height);
                }
            }
            assert(depth < grids.size());
            grids[depth] = grids[depth - 1];
        }
        current_depth = depth;
        break;

    case picross::Solver::Event::KNOWN_LINE:
        break;

    case picross::Solver::Event::DELTA_LINE:
    {
        assert(depth == current_depth);
        const size_t index = line->index();
        ObserverGrid& grid = grids.at(current_depth);
        if (line->type() == picross::Line::ROW)
        {
            for (size_t x = 0u; x < width; ++x)
                if (line->tiles().at(x) != picross::Tile::UNKNOWN)
                {
                    grid.set_tile(x, index, line->tiles().at(x), static_cast<unsigned int>(current_depth));
                }
        }
        else
        {
            for (size_t y = 0u; y < height; ++y)
                if (line->tiles().at(y) != picross::Tile::UNKNOWN)
                {
                    grid.set_tile(index, y, line->tiles().at(y), static_cast<unsigned int>(current_depth));
                }
        }
        break;
    }

    case picross::Solver::Event::SOLVED_GRID:
        break;

    case picross::Solver::Event::INTERNAL_STATE:
        assert(depth == current_depth);
        break;

    default:
        assert(0);  // Unknown Solver::Event
    }

    assert(current_depth < grids.size());
    observer_callback(event, line, depth, misc, grids.at(current_depth));
}

void GridObserver::observer_clear()
{
    assert(!grids.empty());
    grids.erase(grids.begin() + 1, grids.end());
    assert(grids.size() == 1);
    grids[0].reset();
    current_depth = 0u;
}
