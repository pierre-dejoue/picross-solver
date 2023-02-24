#include "grid_window.h"

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


namespace
{
    constexpr ImU32 ColorGridBack = IM_COL32(255, 255, 255, 255);
    constexpr ImU32 ColorGridOutline = IM_COL32(224, 224, 224, 255);

    // Default tile colors (branching depth = 0)
    constexpr ImU32 ColorTileBorder = IM_COL32(20, 90, 116, 255);
    constexpr ImU32 ColorTileFilled = IM_COL32(91, 94, 137, 255);
    constexpr ImU32 ColorTileEmpty = IM_COL32(212, 216, 224, 255);

    // Branching colors
    constexpr ImU32 ColorTileDepth1Border = IM_COL32(116, 20, 90, 255);
    constexpr ImU32 ColorTileDepth1Filled = IM_COL32(137, 91, 94, 255);
    constexpr ImU32 ColorTileDepth1Empty = IM_COL32(224, 212, 216, 255);

    constexpr ImU32 ColorTileDepth2Border = IM_COL32(90, 116, 20, 255);
    constexpr ImU32 ColorTileDepth2Filled = IM_COL32(94, 137, 91, 255);
    constexpr ImU32 ColorTileDepth2Empty = IM_COL32(216, 224, 212, 255);

    // Branching colors (periodical from there)
    constexpr ImU32 ColorTileDepthCyc0Border = IM_COL32(114, 52, 20, 255);
    constexpr ImU32 ColorTileDepthCyc0Filled = IM_COL32(135, 135, 90, 255);
    constexpr ImU32 ColorTileDepthCyc0Empty = IM_COL32(224, 224, 216, 255);

    constexpr ImU32 ColorTileDepthCyc1Border = IM_COL32(52, 20, 114, 255);
    constexpr ImU32 ColorTileDepthCyc1Filled = IM_COL32(135, 90, 135, 255);
    constexpr ImU32 ColorTileDepthCyc1Empty = IM_COL32(224, 216, 224, 255);

    constexpr ImU32 ColorTileDepthCyc2Border = IM_COL32(20, 114, 54, 255);
    constexpr ImU32 ColorTileDepthCyc2Filled = IM_COL32(90, 135, 135, 255);
    constexpr ImU32 ColorTileDepthCyc2Empty = IM_COL32(216, 224, 224, 255);

    // Hidden tile colors (branching depth = 0)
    constexpr ImU32 ColorHiddenTileBorder = IM_COL32(180, 180, 230, 255);
    constexpr ImU32 ColorHiddenTileFilled = IM_COL32(220, 220, 240, 255);
    constexpr ImU32 ColorHiddenTileEmpty = IM_COL32(245, 245, 250, 255);

    unsigned int cycling_color_index(unsigned int depth)
    {
        return depth < 3
            ? depth
            : 3 + ((depth - 3) % 3);
    }

    const ImU32& get_color_tile_border(unsigned int depth = 0)
    {
        static const ImU32 Colors[] = {
            ColorTileBorder,
            ColorTileDepth1Border,
            ColorTileDepth2Border,
            ColorTileDepthCyc0Border,
            ColorTileDepthCyc1Border,
            ColorTileDepthCyc2Border
        };
        return Colors[cycling_color_index(depth)];
    }

    const ImU32& get_color_tile_filled(unsigned int depth = 0)
    {
        static const ImU32 Colors[] = {
            ColorTileFilled,
            ColorTileDepth1Filled,
            ColorTileDepth2Filled,
            ColorTileDepthCyc0Filled,
            ColorTileDepthCyc1Filled,
            ColorTileDepthCyc2Filled
         };
        return Colors[cycling_color_index(depth)];
    }

    const ImU32& get_color_tile_empty(unsigned int depth = 0)
    {
        static const ImU32 Colors[] = {
            ColorTileEmpty,
            ColorTileDepth1Empty,
            ColorTileDepth2Empty,
            ColorTileDepthCyc0Empty,
            ColorTileDepthCyc1Empty,
            ColorTileDepthCyc2Empty
        };
        return Colors[cycling_color_index(depth)];
    }

    size_t get_tile_size(int size_enum)
    {
        // The tile size includes the thickness of the grid (1 pixel), unless for values smaller than 4 in which case the tiles are in contact with each other
        static const std::vector<size_t> TileSizes = { 2, 5, 9, 12, 16 };
        return TileSizes.at(static_cast<size_t>(size_enum));
    }

    size_t background_grid_thickness(size_t tile_size)
    {
        // Note that tile_size includes the grid thickness if the grid outline is visible
        return tile_size > 3 ? 1 : 0;
    }

    void draw_background_grid(ImDrawList* draw_list, ImVec2 tl_corner, size_t tile_size, size_t width, size_t height, bool outline = false)
    {
        const ImVec2 br_corner = ImVec2(tl_corner.x + static_cast<float>(width * tile_size), tl_corner.y + static_cast<float>(height * tile_size));
        draw_list->AddRectFilled(tl_corner, br_corner, ColorGridBack);
        if (background_grid_thickness(tile_size) > 0 && outline)
        {
            for (size_t i = 0u; i <= width; ++i)
            {
                const float x = static_cast<float>(i * tile_size);
                draw_list->AddLine(ImVec2(tl_corner.x + x, tl_corner.y), ImVec2(tl_corner.x + x, br_corner.y), ColorGridOutline);
            }
            for (size_t j = 0u; j <= height; ++j)
            {
                const float y = static_cast<float>(j * tile_size);
                draw_list->AddLine(ImVec2(tl_corner.x, tl_corner.y + y), ImVec2(br_corner.x, tl_corner.y + y), ColorGridOutline);
            }
        }
    }

    void draw_tile(ImDrawList* draw_list, ImVec2 tl_corner, size_t tile_size, size_t i, size_t j, bool filled, bool hidden, unsigned int color_depth = 0, float size_ratio = 1.f, float rounding_ratio = 0.f)
    {
        const bool draw_border = tile_size > 6;
        const auto grid_thickness = background_grid_thickness(tile_size);

        assert(0.f < size_ratio && size_ratio <= 1.f);
        assert(0.f <= rounding_ratio && rounding_ratio <= 1.f);
        const float padding = draw_border ? static_cast<float>(tile_size - 1) * 0.5f * (1.f - size_ratio) : 0.f;
        const float rounding = draw_border ? static_cast<float>(tile_size - 1) * 0.5f * rounding_ratio : 0.f;
        const ImVec2 tl_tile_corner = ImVec2(
            tl_corner.x + static_cast<float>(i * tile_size + grid_thickness) + padding,
            tl_corner.y + static_cast<float>(j * tile_size + grid_thickness) + padding);
        const ImVec2 br_tile_corner = ImVec2(
            tl_corner.x + static_cast<float>((i+1) * tile_size) - padding,
            tl_corner.y + static_cast<float>((j+1) * tile_size) - padding);
        const ImU32 tile_color = hidden
            ? (filled ? ColorHiddenTileFilled : ColorHiddenTileEmpty)
            : (filled ? get_color_tile_filled(color_depth) : get_color_tile_empty(color_depth));
        draw_list->AddRectFilled(tl_tile_corner, br_tile_corner, tile_color, rounding);
        if (draw_border)
        {
            const ImU32 border_color = hidden ? ColorHiddenTileBorder : get_color_tile_border(color_depth);
            draw_list->AddRect(tl_tile_corner, br_tile_corner, border_color, rounding);
        }
    }
} // namespace

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

GridWindow::GridWindow(picross::InputGrid&& grid, std::string_view source, bool start_thread)
    : GridObserver(grid)
    , grid(std::move(grid))
    , title()
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

    const size_t tile_size = get_tile_size(tile_settings.size_enum);

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
        std::swap(solver_thread, std::thread(&GridWindow::solve_picross_grid, this));
        solver_thread_start = false;
    }
    // If solver thread is active
    else if (solver_thread_active)
    {
        if (solver_thread_completed)
        {
            // The solver thread being over does not mean that all observer events have been processed
            solver_thread.join();
            std::swap(solver_thread, std::thread());
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
        bool can_be_erased = false;
        info->visit(can_be_erased);
        if (can_be_erased)
            info.reset();
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
                draw_background_grid(draw_list, grid_tl_corner, tile_size, width, height, true);

                for (size_t i = 0u; i < width; ++i)
                    for (size_t j = 0u; j < height; ++j)
                    {
                        const auto tile = solution.get_tile(i, j);
                        const auto depth = solution.get_depth(i, j);
                        const auto color_depth = animation_settings.show_branching && solver_thread_active && idx + 1 == solutions.size()
                            ? depth
                            : 0;
                        if (tile == picross::Tile::UNKNOWN)
                            continue;
                        const bool filled = tile == picross::Tile::FILLED;
                        const bool hidden = tile_settings.hide_depth_greater ? (depth >= tile_settings.hide_depth_value) : false;
                        draw_tile(draw_list, grid_tl_corner, tile_size, i, j, filled, hidden, color_depth, tile_settings.size_ratio, tile_settings.rounding_ratio);
                    }

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
    const size_t width = grid.width();
    const size_t height = grid.height();
    text_buffer->buffer.clear();
    text_buffer->buffer.appendf("Grid %s\n", picross::str_input_grid_size(grid).c_str());
}

void GridWindow::observer_callback(picross::ObserverEvent event, const picross::Line* line, unsigned int, unsigned int misc, const ObserverGrid& grid)
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
    line_events.emplace_back(event, line, misc, grid);
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
    unsigned count_grids = 0u;

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
          "NON file", "*.non",
          "Bitmap (*.pbm)", "*.pbm" },
        pfd::opt::force_overwrite).result();

    if (!file_path.empty())
    {
        const auto err_handler = [this](std::string_view msg, picross::io::ExitCode)
        {
            std::lock_guard<std::mutex> lock(this->text_buffer->mutex);
            this->text_buffer->buffer.appendf("%s\n", msg);
        };

        const auto solution = (solutions.empty() || !solutions[0].is_completed()) ? std::nullopt : std::optional<picross::OutputGrid>(solutions[0]);
        const auto format = picross::io::picross_file_format_from_filepath(file_path);
        picross::io::save_picross_file(file_path, format, grid, solution, err_handler);
    }
}
