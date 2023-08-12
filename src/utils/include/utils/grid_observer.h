#pragma once

#include <picross/picross.h>

#include <cstddef>
#include <vector>


class ObserverGrid : public picross::OutputGrid
{
    friend class GridObserver;
public:
    ObserverGrid(std::size_t width, std::size_t height, unsigned int depth = 0u, const std::string& name = "");
    ObserverGrid(const OutputGrid& output_grid, unsigned int depth = 0u);
    ObserverGrid(const ObserverGrid& other) = default;
    ObserverGrid(ObserverGrid&& other) noexcept = default;
    ObserverGrid& operator=(const ObserverGrid& other);
    ObserverGrid& operator=(ObserverGrid&& other) noexcept = default;

    unsigned int get_grid_depth() const { return m_depth; }
    unsigned int get_depth(std::size_t x, std::size_t y) const;

private:
    void set_tile(std::size_t x, std::size_t y, picross::Tile t, unsigned int d);

private:
    unsigned int                m_depth;        // Global search depth
    std::vector<unsigned int>   m_depth_grid;   // Search depth per tile
};

class GridObserver
{
public:
    GridObserver(std::size_t width, std::size_t height);
    explicit GridObserver(const picross::InputGrid& grid);
    virtual ~GridObserver() = default;

    void operator()(picross::ObserverEvent event, const picross::Line* line, const picross::ObserverData& data);

protected:
    void observer_clear();

private:
    virtual void observer_callback(picross::ObserverEvent event, const picross::Line* line, const picross::ObserverData& data, const ObserverGrid& grid) = 0;

private:
    std::vector<ObserverGrid> m_grids;
    unsigned int              m_current_depth;
};
