#pragma once

#include <cstddef>
#include <ostream>

#include <picross/picross.h>

#include <optional>


class ConsoleProgressObserver final
{
public:
    explicit ConsoleProgressObserver(std::ostream& ostream);

    void operator()(picross::Solver::Event event, const picross::Line* line, unsigned int depth, unsigned int misc);

private:
    std::ostream&   m_ostream;
    float           m_previous_progress;
};
