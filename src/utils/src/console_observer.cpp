#include <utils/console_observer.h>

#include <cassert>
#include <cstdint>
#include <exception>


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

void ConsoleObserver::observer_callback(picross::Solver::Event event, const picross::Line* line, unsigned int depth, unsigned int misc, const ObserverGrid& grid)
{
    m_ostream << event;
    switch (event)
    {
    case picross::Solver::Event::BRANCHING:
        if (line)
        {
            m_ostream << " NODE";
            m_ostream << " known: " << str_line_full(*line);
            m_ostream << " depth: " << depth;
            m_ostream << " nb_alt: " << misc;
        }
        else
        {
            m_ostream << " EDGE";
            m_ostream << " depth: " << depth;
        }
        break;

    case picross::Solver::Event::KNOWN_LINE:
        assert(line);
        m_ostream << " known: " << str_line_full(*line)
                  << " depth: " << depth
                  << " nb_alt: " << misc;
        break;

    case picross::Solver::Event::DELTA_LINE:
        assert(line);
        m_ostream << " delta: " << str_line_full(*line)
                  << " depth: " << depth
                  << " nb_alt: " << misc;
        break;

    case picross::Solver::Event::SOLVED_GRID:
        m_ostream << " nb " << ++m_nb_solved_grids << std::endl;
        m_ostream << grid;
        break;

    case picross::Solver::Event::INTERNAL_STATE:
        m_ostream << " state: " << misc
                  << " depth: " << depth;
        break;

    case picross::Solver::Event::PROGRESS:
        m_ostream << " progress: " << reinterpret_cast<const float&>(static_cast<const std::uint32_t&>(misc))
                  << " depth: " << depth;
        break;

    default:
        assert(0);  // Unknown Solver::Event
    }
    m_ostream << std::endl;

    // Detect discrepancy with set goal
    if (m_goal.has_value() && depth == 0 && event == picross::Solver::Event::DELTA_LINE)
    {
        assert(line);
        picross::LineId line_id(*line);
        const picross::Line reference_line = m_goal->get_line(line_id);
        if (!picross::are_compatible(grid.get_line(line_id), reference_line))
        {
            m_ostream << "MISMATCH";
            m_ostream << "    goal: " << str_line_full(reference_line);
            m_ostream << std::endl;
        }
    }
}
