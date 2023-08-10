#include "grid_window.h"

#include "draw_grid.h"
#include "err_window.h"
#include "settings.h"

#include <stdutils/string.h>
#include <picross/picross.h>
#include <utils/picross_file_io.h>

#include <imgui.h>
#include <portable-file-dialogs.h>

#include <iostream>
#include <optional>
#include <sstream>
#include <utility>


GridWindow::LineEvent::LineEvent(picross::ObserverEvent event, const picross::Line* line, unsigned int misc, const ObserverGrid& grid)
    : m_event(event)
    , m_line_id()
    , m_misc(misc)
    , m_grid(grid)
{
    if (line) { m_line_id = std::make_optional<picross::LineId>(*line); }
}

struct GridWindow::TextBufferImpl
{
    std::mutex mutex;
    ImGuiTextBuffer buffer;
};

GridWindow::GridWindow(picross::IOGrid&& io_grid, std::string_view source, bool start_thread)
    : GridObserver(io_grid.m_input_grid)
    , grid(std::move(io_grid.m_input_grid))
    , title()
    , goal(std::move(io_grid.m_goal))
    , info()
    , solver_thread()
    , solver_thread_start(start_thread)
    , solver_thread_completed(false)
    , solver_thread_abort(false)
    , solver_thread_stats()
    , solver_progress(0.f)
    , text_buffer(std::make_unique<TextBufferImpl>())
    , solutions()
    , valid_solutions(0)
    , allocate_new_solution(false)
    , tabs()
    , line_events()
    , line_cv()
    , line_mutex()
    , max_nb_solutions(0u)
    , speed(1u)
    , ftl_enabled(false)
    , ftl_req_snapshot(false)
{
    title = std::string(this->grid.name()) + " (" + source.data() + ")";
}

GridWindow::~GridWindow()
{
    solver_thread_abort = true;
    line_cv.notify_all();
    if (solver_thread.joinable())
    {
        std::cerr << "Aborting grid " << grid.name() << "..." << std::endl;
        solver_thread.join();
    }
}

void GridWindow::visit(bool& can_be_erased, Settings& settings)
{
    const size_t width = grid.width();
    const size_t height = grid.height();
    const Settings::Tile& tile_settings = settings.read_tile_settings();
    const Settings::Solver& solver_settings = settings.read_solver_settings();
    const Settings::Animation& animation_settings = settings.read_animation_settings();

    ftl_enabled = animation_settings.ftl;
    if (ftl_enabled)
        ftl_req_snapshot = true;

    auto target_speed = ftl_enabled ? 10 : animation_settings.speed;
    if (speed != target_speed)
    {
        speed = target_speed;
        line_cv.notify_one();
    }

    bool tab_auto_select_last_solution = false;

    max_nb_solutions = solver_settings.limit_solutions ? static_cast<unsigned int>(solver_settings.max_nb_solutions) : 0u;

    const size_t tile_size = picross_grid::get_tile_size(tile_settings.size_enum);

    const float min_win_width  = 20.f + static_cast<float>(width * tile_size);
    const float min_win_height = 100.f + static_cast<float>(height * tile_size);
    ImGui::SetNextWindowSizeConstraints(ImVec2(min_win_width, min_win_height), ImVec2(FLT_MAX, FLT_MAX));

    constexpr ImGuiWindowFlags win_flags = ImGuiWindowFlags_AlwaysAutoResize;
    bool is_window_open = true;
    if (!ImGui::Begin(title.c_str(), &is_window_open, win_flags))
    {
        // Collapsed
        can_be_erased = !is_window_open;
        ImGui::End();
        return;
    }
    can_be_erased = !is_window_open;

    // Solver thread state
    const bool solver_thread_active = solver_thread.joinable();

    // Trigger solver thread
    if (solver_thread_start)
    {
        assert(!solver_thread_active);
        reset_solutions();
        if (info) { info->update_solver_status(0u, 0u, 0.f); }
        solver_thread_completed = false;
        std::thread new_thread(&GridWindow::solve_picross_grid, this);
        std::swap(solver_thread, new_thread);
        solver_thread_start = false;
    }
    // If solver thread is active
    else if (solver_thread_active)
    {
        if (solver_thread_completed)
        {
            // The solver thread being over does not mean that all observer events have been processed
            solver_thread.join();
            std::thread null_thread;
            std::swap(solver_thread, null_thread);
            assert(!solver_thread.joinable());
            std::cerr << "End of solver thread for grid " << grid.name() << std::endl;
            if (info) { info->solver_completed(solver_thread_stats); }
        }

        // Fetch the latest observer events
        std::vector<LineEvent> local_events;
        {
            std::lock_guard<std::mutex> lock(line_mutex);
            if (!line_events.empty())
            {
                local_events.reserve(speed);
                std::swap(local_events, line_events);
            }
        }
        if (!local_events.empty())
        {
            // Unblock the waiting solver thread
            line_cv.notify_one();

            // Process observer event
            const unsigned int added_solutions = process_line_events(local_events);

            // If we opened a new solution tab, make sure it is auto-selected once
            tab_auto_select_last_solution = added_solutions > 0;

            if (info && !solver_thread_completed)
            {
                const auto current_depth = solutions.empty() ? 0u : solutions.back().get_grid_depth();
                info->update_solver_status(valid_solutions, current_depth, solver_progress);
            }
        }
    }
    // Remove the last solution if not valid
    else if (valid_solutions < solutions.size())
    {
        assert(solver_thread_completed);
        assert(line_events.empty());
        solutions.erase(solutions.end() - 1);
        tabs.erase(tabs.end() - 1);
    }

    // Reset button
    if (!solver_thread_active && ImGui::Button("Solve"))
    {
        solver_thread_start = true;     // Triggered for next frame
    }
    else if (solver_thread_active && ImGui::Button("Stop"))
    {
        solver_thread_abort = true;     // Flag to signal the solver thread to stop its current processing
    }

    // Save button
    ImGui::SameLine();
    if (ImGui::Button("Save as"))
    {
        save_grid();
    }

    // Info button
    ImGui::SameLine();
    if (ImGui::Button("Info") && !info)
    {
        info = std::make_unique<GridInfo>(grid);
        {
            std::lock_guard<std::mutex> lock(line_mutex);
            if (solver_thread_completed)
            {
                info->solver_completed(solver_thread_stats);
            }
            else
            {
                const auto current_depth = solutions.empty() ? 0u : solutions.back().get_grid_depth();
                info->update_solver_status(valid_solutions, current_depth, solver_progress);
            }
        }
    }

    // Visit info window
    if (info)
    {
        bool info_can_be_erased = false;
        info->visit(info_can_be_erased);
        if (info_can_be_erased)
            info.reset();
    }

    // Goal button
    if (goal.has_value())
    {
        ImGui::SameLine();
        if (ImGui::Button("Goal") && !goal_win)
        {
            goal_win = std::make_unique<GoalWindow>(*goal, grid.name());
        }
    }

    // Visit goal window
    if (goal_win)
    {
        bool goal_can_be_erased = false;
        goal_win->visit(goal_can_be_erased, settings);
        if (goal_can_be_erased)
            goal_win.reset();
    }

    // Text
    {
        std::lock_guard<std::mutex> lock(text_buffer->mutex);
        ImGui::TextUnformatted(text_buffer->buffer.begin(), text_buffer->buffer.end());
    }

    // Solutions tabs
    {
        if (solutions.empty())
        {
            // No solutions, or not yet computed
            ImGui::End();
            return;
        }
    }

    // Solutions vector was swapped once, therefore can now being accessed without lock
    if (ImGui::BeginTabBar("##TabBar"))
    {
        for (unsigned int idx = 0u; idx < solutions.size(); ++idx)
        {
            const auto last_idx = idx == solutions.size() - 1;
            const ImGuiTabItemFlags tab_flags = (tab_auto_select_last_solution && last_idx) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
            if (ImGui::BeginTabItem(tabs.at(idx).c_str(), nullptr, tab_flags))
            {
                const auto& solution = solutions.at(idx);
                assert(idx + 1 == solutions.size() || solution.is_completed());

                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                assert(draw_list);
                ImVec2 grid_tl_corner = ImGui::GetCursorScreenPos();

                // Background
                picross_grid::draw_background_grid(draw_list, grid_tl_corner, tile_size, width, height, true, tile_settings.five_tile_border);

                // Grid
                const bool color_tiles_by_depth = animation_settings.show_branching && solver_thread_active && idx + 1 == solutions.size();
                picross_grid::draw_grid(draw_list, grid_tl_corner, tile_size, solution, tile_settings, color_tiles_by_depth);

                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

bool GridWindow::abort_solver_thread() const
{
    return solver_thread_abort;
}

void GridWindow::reset_solutions()
{
    assert(!solver_thread.joinable());
    solver_thread_abort = false;
    allocate_new_solution = false;
    valid_solutions = 0;
    solutions.clear();
    tabs.clear();
    observer_clear();
    line_events.clear();
    solver_progress = 0.f;

    // Clear text buffer and print out the grid size
    text_buffer->buffer.clear();
    text_buffer->buffer.appendf("Grid %s\n", picross::str_input_grid_size(grid).c_str());
}

void GridWindow::observer_callback(picross::ObserverEvent event, const picross::Line* line, unsigned int, unsigned int misc, const ObserverGrid& l_grid)
{
    // Filter out events useless to the GUI
    if (event != picross::ObserverEvent::DELTA_LINE && event != picross::ObserverEvent::SOLVED_GRID && event != picross::ObserverEvent::PROGRESS)
        return;

    // In "FTL" mode, we skip all events except the SOLVED_GRID, or if a snapshot is requested by the rendering thread (once per frame)
    if (ftl_enabled && event != picross::ObserverEvent::SOLVED_GRID && !ftl_req_snapshot.exchange(false))
        return;

    std::unique_lock<std::mutex> lock(line_mutex);
    if (line_events.size() >= speed)
    {
        // Wait until the previous line events have been consumed
        line_cv.wait(lock, [this]() -> bool {
            return (this->speed > 0u && this->line_events.empty())
                ||  this->abort_solver_thread();
        });
    }
    line_events.emplace_back(event, line, misc, l_grid);
}

unsigned int GridWindow::process_line_events(std::vector<LineEvent>& events)
{
    assert(!events.empty());

    unsigned int count_allocated = 0u;

    for (auto& event : events)
    {
        if (event.m_event == picross::ObserverEvent::PROGRESS)
        {
            // Only indicative
            solver_progress = reinterpret_cast<const float&>(static_cast<const std::uint32_t&>(event.m_misc));
            continue;
        }
        assert(event.m_event == picross::ObserverEvent::DELTA_LINE || event.m_event == picross::ObserverEvent::SOLVED_GRID);
        if (allocate_new_solution || solutions.empty())
        {
            allocate_new_solution = false;
            count_allocated++;
            solutions.emplace_back(std::move(event.m_grid));
        }
        else
        {
            solutions.back() = std::move(event.m_grid);
        }

        if (event.m_event == picross::ObserverEvent::SOLVED_GRID)
        {
            valid_solutions++;
            allocate_new_solution = true;              // Allocate new solution on next event
        }

        // Adjust number of tabs
        if (tabs.size() < solutions.size())
        {
            tabs.emplace_back("Solution " + std::to_string(tabs.size() + 1));
        }
        assert(tabs.size() == solutions.size());
    }

    return count_allocated;
}

// Solver thread
void GridWindow::solve_picross_grid()
{
    assert(!solver_thread_completed);

    solver_thread_stats = picross::GridStats();

    const auto solver = picross::get_ref_solver();

    // Sanity check of the input data
    const auto [check, check_msg] = picross::check_input_grid(grid);
    if (check)
    {
        solver->set_observer(std::reference_wrapper<GridObserver>(*this));
        solver->set_stats(solver_thread_stats);
        solver->set_abort_function([this]() { return this->abort_solver_thread(); });
        const auto solver_result = solver->solve(grid, max_nb_solutions);
        if (solver_result.status == picross::Solver::Status::ABORTED)
        {
            std::lock_guard<std::mutex> lock(text_buffer->mutex);
            text_buffer->buffer.appendf("Processing aborted\n");
        }
        else if (solver_result.status == picross::Solver::Status::CONTRADICTORY_GRID)
        {
            std::lock_guard<std::mutex> lock(text_buffer->mutex);
            text_buffer->buffer.appendf("Could not solve that grid :-(\n");
        }
        else if (solver_result.solutions.empty())
        {
            std::lock_guard<std::mutex> lock(text_buffer->mutex);
            text_buffer->buffer.appendf("No solution could be found\n");
        }
        else
        {
            // Solutions are filled by the observer
        }
    }
    else
    {
        std::lock_guard<std::mutex> lock(text_buffer->mutex);
        text_buffer->buffer.appendf("Invalid grid. Error message: %s\n", check_msg.c_str());
    }

    solver_thread_completed = true;
}

void GridWindow::save_grid()
{
    const auto file_path = pfd::save_file(
        "Select a file", "",
        { "Picross file", "*.txt",
          "NIN file", "*.nin",
          "NON file", "*.non",
          "Bitmap (*.pbm)", "*.pbm" },
        pfd::opt::force_overwrite).result();

    if (!file_path.empty())
    {
        const auto err_handler = [this](picross::io::ErrorCodeT code, std::string_view msg)
        {
            std::lock_guard<std::mutex> lock(this->text_buffer->mutex);
            this->text_buffer->buffer.appendf("%s %s\n", picross::io::str_error_code(code).c_str(), msg);
        };

        const auto solution = (solutions.empty() || !solutions[0].is_completed()) ? std::nullopt : std::optional<picross::OutputGrid>(solutions[0]);
        const auto format = picross::io::picross_file_format_from_filepath(file_path);
        picross::io::save_picross_file(file_path, format, picross::IOGrid(grid, solution), err_handler);
        std::cout << "User saved file " << file_path << " (format: " << format << ")" << std::endl;
    }
}
