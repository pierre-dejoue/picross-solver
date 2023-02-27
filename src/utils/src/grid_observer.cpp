#include <utils/grid_observer.h>

#include <cassert>
#include <exception>
#include <iostream>


ObserverGrid::ObserverGrid(size_t width, size_t height, unsigned int depth, const std::string& name)
    : OutputGrid(width, height, picross::Tile::UNKNOWN, name)
    , m_depth(depth)
    , m_depth_grid(height * width, 0u)
{}

ObserverGrid& ObserverGrid::operator=(const ObserverGrid& other)
{
    assert(other.m_depth <= m_depth);
    assert(width() == other.width() && height() == other.height());
    static_cast<OutputGrid&>(*this) = static_cast<const OutputGrid&>(other);
    m_depth_grid = other.m_depth_grid;
    // Leave m_depth unchanged
    return *this;
}

void ObserverGrid::set_tile(size_t x, size_t y, picross::Tile t, unsigned int d)
{
    assert(d <= m_depth);
    OutputGrid::set_tile(x, y, t);
    m_depth_grid[x * height() + y] = d;
}

unsigned int ObserverGrid::get_depth(size_t x, size_t y) const
{
    return m_depth_grid[x * height() + y];
}


GridObserver::GridObserver(size_t width, size_t height)
    : m_grids()
    , m_current_depth(0u)
{
    // Grid at depth 0
    m_grids.emplace_back(width, height, m_current_depth);
}

GridObserver::GridObserver(const picross::InputGrid& grid)
    : GridObserver(grid.width(), grid.height())
{
}

void GridObserver::operator()(picross::ObserverEvent event, const picross::Line* line, unsigned int depth, unsigned int misc)
{
    const auto width = m_grids[0].width();
    const auto height = m_grids[0].height();

    switch (event)
    {
    case picross::ObserverEvent::BRANCHING:
        if (!line)
        {
            // BRANCHING EDGE event
            assert(depth > 0u);
            if (depth > m_current_depth)
            {
                assert(depth == m_current_depth + 1u);
                const unsigned int start_fill_depth = static_cast<unsigned int>(m_grids.size());
                for (unsigned int d = start_fill_depth; d <= depth; d++)
                {
                    m_grids.emplace_back(width, height, d);
                }
            }
            assert(depth < m_grids.size());
            m_grids[depth] = m_grids[depth - 1];
        }
        m_current_depth = depth;
        break;

    case picross::ObserverEvent::KNOWN_LINE:
        break;

    case picross::ObserverEvent::DELTA_LINE:
    {
        assert(depth == m_current_depth);
        const size_t index = line->index();
        ObserverGrid& grid = m_grids.at(m_current_depth);
        if (line->type() == picross::Line::ROW)
        {
            for (size_t x = 0u; x < width; ++x)
                if (line->at(x) != picross::Tile::UNKNOWN)
                {
                    grid.set_tile(x, index, line->at(x), m_current_depth);
                }
        }
        else
        {
            for (size_t y = 0u; y < height; ++y)
                if (line->at(y) != picross::Tile::UNKNOWN)
                {
                    grid.set_tile(index, y, line->at(y), m_current_depth);
                }
        }
        break;
    }

    case picross::ObserverEvent::SOLVED_GRID:
        break;

    case picross::ObserverEvent::INTERNAL_STATE:
        break;

    case picross::ObserverEvent::PROGRESS:
        break;

    default:
        assert(0);  // Unknown ObserverEvent
        break;
    }

    assert(m_current_depth < m_grids.size());
    observer_callback(event, line, depth, misc, m_grids.at(m_current_depth));
}

void GridObserver::observer_clear()
{
    assert(!m_grids.empty());
    m_grids.erase(m_grids.begin() + 1, m_grids.end());
    assert(m_grids.size() == 1);
    m_grids[0].reset();
    m_current_depth = 0u;
}
