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
        const auto progress_i = std::make_unique<std::uint32_t>(static_cast<std::uint32_t>(misc));
        const float progress_f = *reinterpret_cast<const float*>(progress_i.get());
        if ((progress_f - m_previous_progress) > 0.001f)
        {
            m_ostream << "Progress: " << progress_f
                      << " (depth: " << depth << ")"
                      << std::endl;
            m_previous_progress = progress_f;
        }
        break;
    }

    default:
        assert(0);  // Unknown ObserverEvent
    }
}
