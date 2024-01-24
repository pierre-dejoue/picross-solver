#pragma once

#include <cstddef>
#include <ostream>

#include <picross/picross.h>
#include <utils/grid_observer.h>

#include <optional>

class ConsoleObserver final : public GridObserver
{
public:
    explicit ConsoleObserver(size_t width, size_t height, std::ostream& ostream);

    // For the purpose of debugging solving algorithms.
    // When set, the observer will automatically report any discrepancy with the set goal at searching depth 0
    void verify_against_goal(const picross::OutputGrid& goal);

private:
    void observer_callback(picross::ObserverEvent event, const picross::Line* line, const picross::ObserverData& data, const ObserverGrid& grid) override;

private:
    std::ostream&                      m_ostream;
    std::optional<picross::OutputGrid> m_goal;
    unsigned int                       m_nb_solved_grids;
};
