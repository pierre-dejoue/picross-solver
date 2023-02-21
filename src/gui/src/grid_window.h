#pragma once

#include <picross/picross.h>
#include <utils/grid_observer.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

class Settings;

class GridWindow : public GridObserver
{
public:
    struct LineEvent
    {
        LineEvent(picross::ObserverEvent event, const picross::Line* line, const ObserverGrid& grid);

        picross::ObserverEvent event;
        std::unique_ptr<picross::Line> event_line;
        ObserverGrid grid;
    };
public:
    GridWindow(picross::InputGrid&& grid, std::string_view source, bool start_thread = true);
    ~GridWindow();
    GridWindow(const GridWindow&) = delete;
    GridWindow& operator=(const GridWindow&) = delete;

    void visit(bool& can_be_erased, Settings& settings);

    bool abort_solver_thread() const;

private:
    void reset_solutions();
    void observer_callback(picross::ObserverEvent event, const picross::Line* line, unsigned int, unsigned int, const ObserverGrid& grid) override;
    unsigned int process_line_events(std::vector<LineEvent>& events);
    void solve_picross_grid();
    void save_grid();

private:
    picross::InputGrid grid;
    std::string title;
    std::thread solver_thread;
    bool solver_thread_start;
    std::atomic<bool> solver_thread_completed;
    std::atomic<bool> solver_thread_abort;
    struct TextBufferImpl;
    std::unique_ptr<TextBufferImpl> text_buffer;
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
