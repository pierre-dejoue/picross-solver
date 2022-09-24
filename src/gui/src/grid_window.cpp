#include "grid_window.h"

#include "err_window.h"
#include "settings.h"

#include <picross/picross_io.h>
#include <utils/bitmap_io.h>
#include <utils/picross_file_io.h>
#include <utils/strings.h>

#include "portable-file-dialogs.h"

#include <iostream>
#include <sstream>
#include <utility>

namespace
{
    constexpr ImU32 ColorGridBack = IM_COL32(255, 255, 255, 255);
    constexpr ImU32 ColorGridOutline = IM_COL32(224, 224, 224, 255);

    // Default tile colors (branching depth = 0)
    constexpr ImU32 ColorTileBorder = IM_COL32(20, 90, 116, 255);
    constexpr ImU32 ColorTileFilled = IM_COL32(91, 94, 137, 255);
    constexpr ImU32 ColorTileEmpty = IM_COL32(216, 216, 224, 255);

    // Branching colors
    constexpr unsigned int DepthColors = 3;

    constexpr ImU32 ColorTileDepth1Border = IM_COL32(116, 20, 90, 255);
    constexpr ImU32 ColorTileDepth1Filled = IM_COL32(137, 91, 94, 255);
    constexpr ImU32 ColorTileDepth1Empty = IM_COL32(224, 216, 216, 255);

    constexpr ImU32 ColorTileDepth2Border = IM_COL32(90, 116, 20, 255);
    constexpr ImU32 ColorTileDepth2Filled = IM_COL32(94, 137, 91, 255);
    constexpr ImU32 ColorTileDepth2Empty = IM_COL32(216, 224, 216, 255);

    const ImU32& get_color_tile_border(unsigned int depth = 0)
    {
        static const ImU32 Colors[3] = { ColorTileBorder, ColorTileDepth1Border, ColorTileDepth2Border };
        return Colors[depth % DepthColors];
    }

    const ImU32& get_color_tile_filled(unsigned int depth = 0)
    {
        static const ImU32 Colors[3] = { ColorTileFilled, ColorTileDepth1Filled, ColorTileDepth2Filled };
        return Colors[depth % DepthColors];
    }

    const ImU32& get_color_tile_empty(unsigned int depth = 0)
    {
        static const ImU32 Colors[3] = { ColorTileEmpty, ColorTileDepth1Empty, ColorTileDepth2Empty };
        return Colors[depth % DepthColors];
    }

    void draw_background_grid(ImDrawList* draw_list, ImVec2 tl_corner, size_t tile_size, size_t width, size_t height, bool outline = false)
    {
        const ImVec2 br_corner = ImVec2(tl_corner.x + static_cast<float>(width * tile_size), tl_corner.y + static_cast<float>(height * tile_size));
        draw_list->AddRectFilled(tl_corner, br_corner, ColorGridBack);
        if (outline)
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

    void draw_tile(ImDrawList* draw_list, ImVec2 tl_corner, size_t tile_size, size_t i, size_t j, bool filled, unsigned int depth = 0, float size_ratio = 1.f, float rounding_ratio = 0.f)
    {
        assert(0.f < size_ratio && size_ratio <= 1.f);
        assert(0.f <= rounding_ratio && rounding_ratio <= 1.f);
        const float padding = static_cast<float>(tile_size - 1) * 0.5f * (1.f - size_ratio);
        const float rounding = static_cast<float>(tile_size - 1) * 0.5f * rounding_ratio;
        const ImVec2 tl_tile_corner = ImVec2(
            tl_corner.x + static_cast<float>(i * tile_size + 1) + padding,
            tl_corner.y + static_cast<float>(j * tile_size + 1) + padding);
        const ImVec2 br_tile_corner = ImVec2(
            tl_corner.x + static_cast<float>((i+1) * tile_size) - padding,
            tl_corner.y + static_cast<float>((j+1) * tile_size) - padding);
        draw_list->AddRectFilled(tl_tile_corner, br_tile_corner, filled ? get_color_tile_filled(depth) : get_color_tile_empty(depth), rounding);
        draw_list->AddRect(tl_tile_corner, br_tile_corner, get_color_tile_border(depth), rounding);
    }
} // Anonymous namespace

GridWindow::LineEvent::LineEvent(picross::Solver::Event event, const picross::Line* delta, const ObserverGrid& grid)
    : event(event)
    , delta()
    , grid(grid)
{
    if (delta)
    {
        this->delta = std::make_unique<picross::Line>(*delta);
    }
}

GridWindow::GridWindow(picross::InputGrid&& grid, const std::string& source, bool start_thread)
    : GridObserver(grid.cols.size(), grid.rows.size())
    , grid(std::move(grid))
    , title()
    , solver_thread()
    , solver_thread_start(start_thread)
    , solver_thread_completed(false)
    , solver_thread_abort(false)
    , text_buffer_mutex()
    , text_buffer()
    , solutions()
    , valid_solutions(0)
    , allocate_new_solution(false)
    , tabs()
    , line_events()
    , line_cv()
    , line_mutex()
    , max_nb_solutions(0u)
    , speed(1u)
{
    title = this->grid.name + " (" + source + ")";
}

GridWindow::~GridWindow()
{
    solver_thread_abort = true;
    line_cv.notify_all();
    if (solver_thread.joinable())
    {
        std::cerr << "Aborting grid " << grid.name << "..." << std::endl;
        solver_thread.join();
    }
}

void GridWindow::visit(bool& canBeErased, Settings& settings)
{
    const size_t width = grid.cols.size();
    const size_t height = grid.rows.size();
    const Settings::Tile& tile_settings = settings.read_tile_settings();
    const Settings::Solver& solver_settings = settings.read_solver_settings();
    const Settings::Animation& animation_settings = settings.read_animation_settings();

    if (animation_settings.speed != speed)
    {
        speed = animation_settings.speed;
        line_cv.notify_one();
    }

    bool tab_auto_select_last_solution = false;

    max_nb_solutions = solver_settings.limit_solutions ? static_cast<unsigned int>(solver_settings.max_nb_solutions) : 0u;

    static const std::vector<size_t> TileSizes = { 12, 18, 24 };
    const size_t tile_size = TileSizes.at(static_cast<size_t>(tile_settings.size_enum));

    ImGui::SetNextWindowSize(ImVec2(20 + static_cast<float>(width * tile_size), 100 + static_cast<float>(height * tile_size)));

    bool isWindowOpen = true;
    if (!ImGui::Begin(title.c_str(), &isWindowOpen))
    {
        // Collapsed
        canBeErased = !isWindowOpen;
        ImGui::End();
        return;
    }
    canBeErased = !isWindowOpen;

    // Solver thread state
    const bool solver_thread_active = solver_thread.joinable();

    // Trigger solver thread
    if (solver_thread_start)
    {
        assert(!solver_thread_active);
        reset_solutions();
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
            std::cerr << "End of solver thread for grid " << grid.name << std::endl;
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

    // Text
    {
        std::lock_guard<std::mutex> lock(text_buffer_mutex);
        ImGui::TextUnformatted(text_buffer.begin(), text_buffer.end());
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
                assert(idx + 1 == solutions.size() || solution.is_solved());

                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                assert(draw_list);
                ImVec2 grid_tl_corner = ImGui::GetCursorScreenPos();
                draw_background_grid(draw_list, grid_tl_corner, tile_size, width, height, true);

                for (size_t i = 0u; i < width; ++i)
                    for (size_t j = 0u; j < height; ++j)
                    {
                        const auto tile = solution.get(i, j);
                        const auto depth = animation_settings.show_branching && solver_thread_active && idx + 1 == solutions.size()
                            ? solution.get_depth(i, j)
                            : 0;
                        if (tile == picross::Tile::UNKNOWN)
                            continue;
                        draw_tile(draw_list, grid_tl_corner, tile_size, i, j, tile == picross::Tile::ONE, depth, tile_settings.size_ratio, tile_settings.rounding_ratio);
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

    // Clear text buffer and print out the grid size
    const size_t width = grid.cols.size();
    const size_t height = grid.rows.size();
    text_buffer.clear();
    text_buffer.appendf("Grid %s\n", grid_size_str(grid).c_str());
}

void GridWindow::observer_callback(picross::Solver::Event event, const picross::Line* delta, unsigned int depth, const ObserverGrid& grid)
{
    std::unique_lock<std::mutex> lock(line_mutex);
    if (line_events.size() >= speed)
    {
        // Wait until the previous line events have been consumed
        line_cv.wait(lock, [this]() -> bool {
            return (this->speed > 0u && this->line_events.empty())
                ||  this->abort_solver_thread();
        });
    }
    line_events.emplace_back(event, delta, grid);
}

unsigned int GridWindow::process_line_events(std::vector<LineEvent>& events)
{
    assert(!events.empty());

    unsigned int count_allocated = 0u;

    for (auto& event : events)
    {
        if (allocate_new_solution || solutions.empty())
        {
            allocate_new_solution = false;
            count_allocated++;
            solutions.emplace_back(std::move(event.grid));
        }
        else
        {
            solutions.back() = std::move(event.grid);
        }

        switch (event.event)
        {
        case picross::Solver::Event::BRANCHING:
            break;

        case picross::Solver::Event::DELTA_LINE:
            break;

        case picross::Solver::Event::SOLVED_GRID:
            valid_solutions++;
            allocate_new_solution = true;              // Allocate new solution on next event
            break;

        default:
            throw std::invalid_argument("Unknown Solver::Event");
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

    const auto solver = picross::get_ref_solver();
    unsigned count_grids = 0u;

    // Sanity check of the input data
    bool check;
    std::string check_msg;
    std::tie(check, check_msg) = picross::check_grid_input(grid);
    if (check)
    {
        solver->set_observer(std::reference_wrapper<GridObserver>(*this));
        solver->set_abort_function([this]() { return this->abort_solver_thread(); });
        picross::Solver::Status status;
        std::vector<picross::OutputGrid> local_solutions;
        std::tie (status, local_solutions) = solver->solve(grid, max_nb_solutions);
        if (status == picross::Solver::Status::ABORTED)
        {
            std::lock_guard<std::mutex> lock(text_buffer_mutex);
            text_buffer.appendf("Processing aborted\n");
        }
        else if (status == picross::Solver::Status::CONTRADICTORY_GRID)
        {
            std::lock_guard<std::mutex> lock(text_buffer_mutex);
            text_buffer.appendf("Could not solve that grid :-(\n");
        }
        else if (local_solutions.empty())
        {
            std::lock_guard<std::mutex> lock(text_buffer_mutex);
            text_buffer.appendf("No solution could be found\n");
        }
        else
        {
            // Solutions are filled by the observer
        }
    }
    else
    {
        std::lock_guard<std::mutex> lock(text_buffer_mutex);
        text_buffer.appendf("Invalid grid. Error message: %s\n", check_msg.c_str());
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
            std::lock_guard<std::mutex> lock(this->text_buffer_mutex);
            this->text_buffer.appendf("%s\n", msg);
        };

        const std::string ext = str_tolower(file_extension(file_path));
        if (ext == "pbm")
        {
            if (!solutions.empty())
            {
                export_bitmap_pbm(file_path, solutions[0], err_handler);
            }
        }
        else
        {
            save_picross_file(file_path, grid, err_handler);
        }
    }
}
