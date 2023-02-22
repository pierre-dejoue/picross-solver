#pragma once

#include <picross/picross.h>

#include <atomic>
#include <mutex>
#include <string>
#include <utility>
#include <vector>


class GridInfo
{
public:
    GridInfo(const picross::InputGrid& grid);
    GridInfo(const GridInfo&) = delete;
    GridInfo& operator=(const GridInfo&) = delete;

    void visit(bool& can_be_erased);

    void update_solver_status(unsigned int current_depth, float progress);
    void solver_completed(const picross::GridStats& stats);

private:
    void refresh_stats_info();
    std::string info_as_string(unsigned int active_sections) const;

public:
    using InfoMap = std::vector<std::pair<std::string, std::string>>;

private:
    struct SolverStats
    {
        bool                    m_ongoing = false;
        picross::GridStats      m_grid_stats{};
        unsigned int            m_current_depth = 0u;
        float                   m_progress = 0.f;
        //picross::Solver::Status m_status = picross::Solver::Status::OK;     // Irrelevant if m_ongoing == true
    };


private:
    const picross::InputGrid& grid;
    std::string title;
    InfoMap grid_metadata;
    SolverStats solver_stats;
    std::mutex solver_stats_mutex;
    std::atomic<bool> solver_stats_flag;
    InfoMap solver_stats_info;
};
