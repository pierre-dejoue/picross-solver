#pragma once

#include <picross/picross.h>
#include <utils/grid_observer.h>

#include <imgui.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

class Settings;

class GridWindow : public GridObserver
{
public:
    struct LineEvent
    {
        LineEvent(picross::Solver::Event event, const picross::Line* delta, const ObserverGrid& grid);

        picross::Solver::Event event;
        std::unique_ptr<picross::Line> delta;
        ObserverGrid grid;
    };
public:
    GridWindow(picross::InputGrid&& grid, const std::string& source);
    ~GridWindow();
    GridWindow(const GridWindow&) = delete;
    GridWindow& operator=(const GridWindow&) = delete;

    void visit(bool& canBeErased, Settings& settings);

    bool abort_solver_thread() const;

private:
    void reset_solutions();
    void observer_callback(picross::Solver::Event event, const picross::Line* delta, unsigned int depth, const ObserverGrid& grid) override;
    unsigned int process_line_events(std::vector<LineEvent>& events);
    void solve_picross_grid();

private:
    picross::InputGrid grid;
    std::string title;
    std::thread solver_thread;
    bool solver_thread_start;
    std::atomic<bool> solver_thread_completed;
    std::atomic<bool> solver_thread_abort;
    std::mutex text_buffer_mutex;
    ImGuiTextBuffer text_buffer;
    std::vector<ObserverGrid> solutions;
    unsigned int valid_solutions;
    bool allocate_new_solution;
    std::vector<std::string> tabs;
    std::vector<LineEvent> line_events;
    std::condition_variable line_cv;
    std::mutex line_mutex;
    unsigned int max_nb_solutions;
    std::atomic<int> speed;
};
