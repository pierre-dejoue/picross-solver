#include "grid_window.h"

#include "err_window.h"
#include "settings.h"

#include <sstream>
#include <utility>

namespace
{
    constexpr unsigned int GridTile = 24;

    constexpr ImU32 ColorGridBack = IM_COL32(255, 255, 255, 255);
    constexpr ImU32 ColorGridOutline = IM_COL32(224, 224, 224, 255);

    constexpr ImU32 ColorTileBorder = IM_COL32(20, 90, 116, 255);
    constexpr ImU32 ColorTileFilled = IM_COL32(91, 94, 137, 255);
    constexpr ImU32 ColorTileEmpty = IM_COL32(216, 216, 216, 255);

    void draw_background_grid(ImDrawList* draw_list, ImVec2 tl_corner, size_t width, size_t height, bool outline = false)
    {
        const ImVec2 br_corner = ImVec2(tl_corner.x + static_cast<float>(width * GridTile), tl_corner.y + static_cast<float>(height * GridTile));
        draw_list->AddRectFilled(tl_corner, br_corner, ColorGridBack);
        if (outline)
        {
            for (size_t i = 0u; i <= width; ++i)
            {
                const float x = static_cast<float>(i * GridTile);
                draw_list->AddLine(ImVec2(tl_corner.x + x, tl_corner.y), ImVec2(tl_corner.x + x, br_corner.y), ColorGridOutline);
            }
            for (size_t j = 0u; j <= height; ++j)
            {
                const float y = static_cast<float>(j * GridTile);
                draw_list->AddLine(ImVec2(tl_corner.x, tl_corner.y + y), ImVec2(br_corner.x, tl_corner.y + y), ColorGridOutline);
            }
        }
    }

    void draw_tile(ImDrawList* draw_list, ImVec2 tl_corner, size_t i, size_t j, ImU32 fill_color, float size_ratio = 1.f, float rounding_ratio = 0.f)
    {
        assert(0.f < size_ratio && size_ratio <= 1.f);
        assert(0.f <= rounding_ratio && rounding_ratio <= 1.f);
        const float padding = static_cast<float>(GridTile - 1) * 0.5f * (1.f - size_ratio);
        const float rounding = static_cast<float>(GridTile - 1) * 0.5f * rounding_ratio;
        const ImVec2 tl_tile_corner = ImVec2(
            tl_corner.x + static_cast<float>(i * GridTile + 1) + padding,
            tl_corner.y + static_cast<float>(j * GridTile + 1) + padding);
        const ImVec2 br_tile_corner = ImVec2(
            tl_corner.x + static_cast<float>((i+1) * GridTile) - padding,
            tl_corner.y + static_cast<float>((j+1) * GridTile) - padding);
        draw_list->AddRectFilled(tl_tile_corner, br_tile_corner, fill_color, rounding);
        draw_list->AddRect(tl_tile_corner, br_tile_corner, ColorTileBorder, rounding);
    }
} // Anonymous namespace

GridWindow::LineEvent::LineEvent(picross::Solver::Event event, const picross::Line* delta, const picross::OutputGrid& grid)
    : event(event)
    , delta()
    , grid(grid)
{
    if (delta)
    {
        this->delta = std::make_unique<picross::Line>(*delta);
    }
}

GridWindow::GridWindow(picross::InputGrid&& grid, const std::string& source)
    : GridObserver(grid.cols.size(), grid.rows.size())
    , grid(std::move(grid))
    , title()
    , solver_thread()
    , text_buffer_mutex()
    , text_buffer()
    , solutions()
    , alloc_new_solution(true)
    , tabs()
    , line_event()
    , line_cv()
    , line_mutex()
{
    title = this->grid.name + " (" + source + ")";
    std::swap(solver_thread, std::thread(&GridWindow::solve_picross_grid, this));
}

GridWindow::~GridWindow()
{
    solver_thread.join();
}

void GridWindow::visit(bool& canBeErased, Settings& settings)
{
    const size_t width = grid.cols.size();
    const size_t height = grid.rows.size();

    ImGui::SetNextWindowSize(ImVec2(20 + static_cast<float>(width * GridTile), 100 + static_cast<float>(height * GridTile)), ImGuiCond_Once);

    bool isWindowOpen = true;
    if (!ImGui::Begin(title.c_str(), &isWindowOpen))
    {
        // Collapsed
        canBeErased = !isWindowOpen;
        ImGui::End();
        return;
    }
    canBeErased = !isWindowOpen;

    // Process last observer event
    const bool observer_event = [this]()
    {
        std::lock_guard<std::mutex> lock(line_mutex);
        return static_cast<bool>(line_event);
    }();
    if (observer_event)
    {
        process_line_event();
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
            const Settings::Tile& tile_settings = settings.read_tile_settings();
            if (ImGui::BeginTabItem(tabs.at(idx).c_str()))
            {
                const auto& solution = solutions.at(idx);
                assert(idx + 1 == solutions.size() || solution.is_solved());

                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                assert(draw_list);
                ImVec2 grid_tl_corner = ImGui::GetCursorScreenPos();
                draw_background_grid(draw_list, grid_tl_corner, width, height, true);

                for (size_t i = 0u; i < width; ++i)
                    for (size_t j = 0u; j < height; ++j)
                    {
                        const auto tile = solution.get(i, j);
                        if (tile == picross::Tile::UNKNOWN)
                            continue;
                        const auto fill_color = tile == picross::Tile::ONE ? ColorTileFilled : ColorTileEmpty;
                        draw_tile(draw_list, grid_tl_corner, i, j, fill_color, tile_settings.size_ratio, tile_settings.rounding_ratio);
                    }

                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void GridWindow::observer_callback(picross::Solver::Event event, const picross::Line* delta, unsigned int depth, const picross::OutputGrid& grid)
{
    // Wait until the previous line event has been consumed
    std::unique_lock<std::mutex> lock(line_mutex);
    line_cv.wait(lock, [this] { return !this->line_event; });

    // Store new event
    line_event = std::make_unique<LineEvent>(event, delta, grid);
}

void GridWindow::process_line_event()
{
    assert(line_event);

    // Initial solution
    if (alloc_new_solution)
    {
        alloc_new_solution = false;
        solutions.emplace_back(std::move(line_event->grid));
    }
    else
    {
        solutions.back() = std::move(line_event->grid);
    }

    switch (line_event->event)
    {
    case picross::Solver::Event::BRANCHING:
        break;

    case picross::Solver::Event::DELTA_LINE:
        break;

    case picross::Solver::Event::SOLVED_GRID:
        alloc_new_solution = true;              // Allocate new solution on next event
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

    // Ready for next event
    std::lock_guard<std::mutex> lock(line_mutex);
    line_event.reset();
    line_cv.notify_one();
}

// Solver thread
void GridWindow::solve_picross_grid()
{
    const auto solver = picross::get_ref_solver();
    unsigned count_grids = 0u;

    // Sanity check of the input data
    bool check;
    std::string check_msg;
    std::tie(check, check_msg) = picross::check_grid_input(grid);
    if (check)
    {
        solver->set_observer(std::reference_wrapper<GridObserver>(*this));
        std::vector<picross::OutputGrid> local_solutions = solver->solve(grid);
        if (local_solutions.empty())
        {
            std::lock_guard<std::mutex> lock(text_buffer_mutex);
            text_buffer.appendf("Could not solve that grid :-(\n");
        }
        else
        {
            // Solutions are filled by the observer
            assert(local_solutions.size() == solutions.size());
        }
    }
    else
    {
        std::lock_guard<std::mutex> lock(text_buffer_mutex);
        text_buffer.appendf("Invalid grid. Error message: %s\n", check_msg.c_str());
    }
}
