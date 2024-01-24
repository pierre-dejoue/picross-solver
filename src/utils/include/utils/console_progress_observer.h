#pragma once

#include <cstddef>
#include <ostream>

#include <picross/picross.h>

#include <optional>

class ConsoleProgressObserver final
{
public:
    explicit ConsoleProgressObserver(std::ostream& ostream);

    void operator()(picross::ObserverEvent event, const picross::Line* line, const picross::ObserverData& data);

private:
    std::ostream&   m_ostream;
    float           m_previous_progress;
};
