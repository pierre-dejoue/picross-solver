#include "goal_window.h"

#include "draw_grid.h"
#include "settings.h"

#include <imgui.h>


GoalWindow::GoalWindow(const picross::OutputGrid& goal, std::string_view name)
    : grid(goal)
    , title(std::string(name) + " Goal")
{
}

void GoalWindow::visit(bool& can_be_erased, Settings& settings)
{
    const size_t width = grid.width();
    const size_t height = grid.height();

    const Settings::Tile& tile_settings = settings.read_tile_settings();

    const size_t tile_size = picross_grid::get_tile_size(tile_settings.size_enum);

    const float min_win_width  = 20.f + static_cast<float>(width * tile_size);
    const float min_win_height = 80.f + static_cast<float>(height * tile_size);
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

    ImGui::Text("Grid %s\n", picross::str_output_grid_size(grid).c_str());

    // Solutions vector was swapped once, therefore can now being accessed without lock
    if (ImGui::BeginTabBar("##TabBar"))
    {
        const ImGuiTabItemFlags tab_flags = ImGuiTabItemFlags_SetSelected;
        if (ImGui::BeginTabItem("Goal", nullptr, tab_flags))
        {
            assert(grid.is_completed());
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            assert(draw_list);
            ImVec2 grid_tl_corner = ImGui::GetCursorScreenPos();

            // Background
            picross_grid::draw_background_grid(draw_list, grid_tl_corner, tile_size, width, height, true, tile_settings.five_tile_border);

            // Grid
            picross_grid::draw_grid(draw_list, grid_tl_corner, tile_size, grid, tile_settings);

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}
