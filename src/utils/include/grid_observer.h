#pragma once

#include <cstddef>
#include <vector>

#include <picross/picross.h>

class GridObserver
{
public:
    explicit GridObserver(size_t width, size_t height);
    virtual ~GridObserver() = default;

    void operator()(picross::Solver::Event event, const picross::Line* delta, unsigned int depth);

private:
    virtual void observer_callback(picross::Solver::Event event, const picross::Line* delta, unsigned int depth, const picross::OutputGrid& grid) = 0;

private:
    std::vector<picross::OutputGrid> grids;
    size_t current_depth;
};
