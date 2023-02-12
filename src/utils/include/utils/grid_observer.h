#pragma once

#include <cstddef>
#include <vector>

#include <picross/picross.h>

class ObserverGrid : public picross::OutputGrid
{
    friend class GridObserver;
public:
    ObserverGrid(std::size_t width, std::size_t height, const std::string& name = "");

    unsigned int get_depth(std::size_t x, std::size_t y) const;

private:
    void set_tile(std::size_t x, std::size_t y, picross::Tile t, unsigned int d);

    std::vector<unsigned int> depth_grid;
};

class GridObserver
{
public:
    GridObserver(std::size_t width, std::size_t height);
    explicit GridObserver(const picross::InputGrid& grid);
    virtual ~GridObserver() = default;

    void operator()(picross::ObserverEvent event, const picross::Line* line, unsigned int depth, unsigned int misc);

protected:
    void observer_clear();

private:
    virtual void observer_callback(picross::ObserverEvent event, const picross::Line* line, unsigned int depth, unsigned int misc, const ObserverGrid& grid) = 0;

private:
    std::vector<ObserverGrid> grids;
    std::size_t current_depth;
};
