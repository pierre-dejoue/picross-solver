#pragma once

#include <picross/picross.h>
#include <grid_observer.h>

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
        LineEvent(picross::Solver::Event event, const picross::Line* delta, const picross::OutputGrid& grid);

        picross::Solver::Event event;
        std::unique_ptr<picross::Line> delta;
        picross::OutputGrid grid;
    };
public:
    GridWindow(picross::InputGrid&& grid, const std::string& source);
    ~GridWindow();
    GridWindow(const GridWindow&) = delete;
    GridWindow& operator=(const GridWindow&) = delete;

    void visit(bool& canBeErased, Settings& settings);

private:
    void observer_callback(picross::Solver::Event event, const picross::Line* delta, unsigned int depth, const picross::OutputGrid& grid) override;
    void process_line_event(LineEvent* event);
    void solve_picross_grid();

private:
    picross::InputGrid grid;
    std::string title;
    std::thread solver_thread;
    std::mutex text_buffer_mutex;
    ImGuiTextBuffer text_buffer;
    std::vector<picross::OutputGrid> solutions;
    bool alloc_new_solution;
    std::vector<std::string> tabs;
    std::unique_ptr<LineEvent> line_event;
    std::condition_variable line_cv;
    std::mutex line_mutex;
};
