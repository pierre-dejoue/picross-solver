#include <utils/console_observer.h>

#include <cassert>
#include <cstdint>
#include <exception>
#include <memory>

ConsoleObserver::ConsoleObserver(size_t width, size_t height, std::ostream& ostream)
    : GridObserver(width, height)
    , m_ostream(ostream)
    , m_goal()
    , m_nb_solved_grids(0u)
{
}

void ConsoleObserver::verify_against_goal(const picross::OutputGrid& goal)
{
    m_goal = goal;
}

void ConsoleObserver::observer_callback(picross::ObserverEvent event, const picross::Line* line, const picross::ObserverData& data, const ObserverGrid& grid)
{
    m_ostream << event;
    switch (event)
    {
    case picross::ObserverEvent::BRANCHING:
        if (line)
        {
            m_ostream << " NODE";
            m_ostream << " known: " << picross::str_line_full(*line);
            m_ostream << " depth: " << data.m_depth;
            m_ostream << " nb_alt: " << data.m_misc_i;
        }
        else
        {
            m_ostream << " EDGE";
            m_ostream << " depth: " << data.m_depth;
        }
        break;

    case picross::ObserverEvent::KNOWN_LINE:
        assert(line);
        m_ostream << " known: " << picross::str_line_full(*line)
                  << " depth: " << data.m_depth
                  << " nb_alt: " << data.m_misc_i;
        break;

    case picross::ObserverEvent::DELTA_LINE:
        assert(line);
        m_ostream << " delta: " << picross::str_line_full(*line)
                  << " depth: " << data.m_depth
                  << " nb_alt: " << data.m_misc_i;
        break;

    case picross::ObserverEvent::SOLVED_GRID:
        m_ostream << " nb " << ++m_nb_solved_grids << std::endl;
        m_ostream << grid;
        break;

    case picross::ObserverEvent::INTERNAL_STATE:
        m_ostream << " state: " << picross::str_solver_internal_state(data.m_misc_i)
                  << " depth: " << data.m_depth;
        break;

    case picross::ObserverEvent::PROGRESS:
    {
        const float& progress = data.m_misc_f;
        m_ostream << " progress: " << progress
                  << " depth: " << data.m_depth;
        break;
    }

    default:
        assert(0);  // Unknown ObserverEvent
    }
    m_ostream << std::endl;

    // Detect discrepancy with set goal
    if (m_goal.has_value() && data.m_depth == 0 && event == picross::ObserverEvent::DELTA_LINE)
    {
        assert(line);
        picross::LineId line_id(*line);
        const picross::Line reference_line = m_goal->get_line(line_id);
        if (!picross::are_compatible(grid.get_line(line_id), reference_line))
        {
            m_ostream << "MISMATCH";
            m_ostream << "    goal: " << picross::str_line_full(reference_line);
            m_ostream << std::endl;
        }
    }
}
