#include <utils/console_progress_observer.h>

#include <cassert>
#include <cstdint>
#include <exception>


ConsoleProgressObserver::ConsoleProgressObserver(std::ostream& ostream)
    : m_ostream(ostream)
    , m_previous_progress(0.f)
{
}

void ConsoleProgressObserver::operator()(picross::ObserverEvent event, const picross::Line*, unsigned int depth, unsigned int misc)
{
    switch (event)
    {
    case picross::ObserverEvent::BRANCHING:
    case picross::ObserverEvent::KNOWN_LINE:
    case picross::ObserverEvent::DELTA_LINE:
    case picross::ObserverEvent::SOLVED_GRID:
    case picross::ObserverEvent::INTERNAL_STATE:
        // Ignore
        break;

    case picross::ObserverEvent::PROGRESS:
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
        assert(0);  // Unknown ObserverEvent
    }
}
