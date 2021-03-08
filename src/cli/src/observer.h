#pragma once

#include <cstddef>
#include <ostream>
#include <vector>

#include <picross/picross.h>


class ConsoleObserver
{
public:
    explicit ConsoleObserver(size_t width, size_t height, std::ostream& ostream);

    void operator()(picross::Solver::Event event, const picross::Line* delta, unsigned int index, unsigned int depth);

private:
    std::ostream& ostream;
    std::vector<picross::OutputGrid> grids;
    size_t current_depth;
};
