#pragma once

#include <cstddef>
#include <ostream>

#include <picross/picross.h>

#include <grid_observer.h>


class ConsoleObserver final : public GridObserver
{
public:
    explicit ConsoleObserver(size_t width, size_t height, std::ostream& ostream);

private:
    void callback(picross::Solver::Event event, const picross::Line* delta, unsigned int index, unsigned int depth, const picross::OutputGrid& grid) override;

private:
    std::ostream& ostream;
};
