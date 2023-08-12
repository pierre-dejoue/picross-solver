#include <utils/console_progress_observer.h>

#include <cassert>
#include <cstdint>
#include <exception>


ConsoleProgressObserver::ConsoleProgressObserver(std::ostream& ostream)
    : m_ostream(ostream)
    , m_previous_progress(0.f)
{
}

void ConsoleProgressObserver::operator()(picross::ObserverEvent event, const picross::Line*, const picross::ObserverData& data)
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
        const float& progress = data.m_misc_f;
        if ((progress - m_previous_progress) > 0.001f)
        {
            m_ostream << "Progress: " << progress
                      << " (depth: " << data.m_depth << ")"
                      << std::endl;
            m_previous_progress = progress;
        }
        break;
    }

    default:
        assert(0);  // Unknown ObserverEvent
    }
}
