#include <utils/console_progress_observer.h>

#include <cassert>
#include <cstdint>
#include <exception>


ConsoleProgressObserver::ConsoleProgressObserver(std::ostream& ostream)
    : m_ostream(ostream)
    , m_previous_progress(0.f)
{
}

void ConsoleProgressObserver::operator()(picross::Solver::Event event, const picross::Line*, unsigned int depth, unsigned int misc)
{
    switch (event)
    {
    case picross::Solver::Event::BRANCHING:
    case picross::Solver::Event::KNOWN_LINE:
    case picross::Solver::Event::DELTA_LINE:
    case picross::Solver::Event::SOLVED_GRID:
    case picross::Solver::Event::INTERNAL_STATE:
        // Ignore
        break;

    case picross::Solver::Event::PROGRESS:
    {
        const float current_progress = reinterpret_cast<const float&>(static_cast<const std::uint32_t&>(misc));
        if ((current_progress - m_previous_progress) > 0.001f)
        {
            m_ostream << "Progress: " << current_progress
                      << " (depth: " << depth << ")"
                      << std::endl;
            m_previous_progress = current_progress;
        }
        break;
    }

    default:
        assert(0);  // Unknown Solver::Event
    }
}
