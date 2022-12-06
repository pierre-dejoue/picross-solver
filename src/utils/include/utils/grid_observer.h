#pragma once

#include <cstddef>
#include <vector>

#include <picross/picross.h>

class ObserverGrid : public picross::OutputGrid
{
public:
    ObserverGrid(size_t width, size_t height, const std::string& name = "");

    void set(size_t x, size_t y, picross::Tile t, unsigned int d);
    unsigned int get_depth(size_t x, size_t y) const;

private:
    std::vector<unsigned int>           depth_grid;
};

class GridObserver
{
public:
    GridObserver(size_t width, size_t height);
    explicit GridObserver(const picross::InputGrid& grid);
    virtual ~GridObserver() = default;

    void operator()(picross::Solver::Event event, const picross::Line* delta, unsigned int depth);

protected:
    void observer_clear();

private:
    virtual void observer_callback(picross::Solver::Event event, const picross::Line* delta, unsigned int depth, const ObserverGrid& grid) = 0;

private:
    std::vector<ObserverGrid> grids;
    size_t current_depth;
};
