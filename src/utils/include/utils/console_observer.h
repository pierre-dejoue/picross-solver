#pragma once

#include <cstddef>
#include <ostream>

#include <picross/picross.h>
#include <utils/grid_observer.h>


class ConsoleObserver final : public GridObserver
{
public:
    explicit ConsoleObserver(size_t width, size_t height, std::ostream& ostream);

private:
    void observer_callback(picross::Solver::Event event, const picross::Line* delta, unsigned int depth, unsigned int misc, const ObserverGrid& grid) override;

private:
    std::ostream& ostream;
    unsigned int nb_solved_grids;
};
