#include "settings_window.h"

#include "settings.h"

#include <imgui_wrap.h>
#include "imgui_helpers.h"

#include <cassert>

SettingsWindow::SettingsWindow(Settings& settings)
    : settings(settings)
    , title("Settings")
    , animation()
{
}

void SettingsWindow::visit(bool& can_be_erased, const WindowLayout& win_pos_sz)
{
    can_be_erased = false;        // Cannot close

    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 300), ImVec2(FLT_MAX, 600));

    ImGui::SetNextWindowPosAndSize(win_pos_sz, ImGuiCond_Once);
    constexpr ImGuiWindowFlags win_flags = ImGuiWindowFlags_AlwaysAutoResize
                                         | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin(title.c_str(), nullptr, win_flags))
    {
        // Collapsed
        ImGui::End();
        return;
    }

    Settings::Tile* tile_settings = settings.get_tile_settings();
    if (tile_settings)
    {
        const auto& limits = settings.read_tile_settings_limits();

        ImGui::BulletText("Tiles");
        ImGui::Indent();

        ImGui::Text("Size: ");
        ImGui::SameLine();
        ImGui::RadioButton("XS", &tile_settings->size_enum, 0);
        ImGui::SameLine();
        ImGui::RadioButton("S", &tile_settings->size_enum, 1);
        ImGui::SameLine();
        ImGui::RadioButton("M", &tile_settings->size_enum, 2);
        ImGui::SameLine();
        ImGui::RadioButton("L", &tile_settings->size_enum, 3);
        ImGui::SameLine();
        ImGui::RadioButton("XL", &tile_settings->size_enum, 4);

        //ImGui::SliderFloat("Rounding ratio", &tile_settings->rounding_ratio, limits.rounding_ratio.min, limits.rounding_ratio.max, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        //ImGui::SliderFloat("Size ratio", &tile_settings->size_ratio, limits.size_ratio.min, limits.size_ratio.max, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Checkbox("Five tile outline", &tile_settings->five_tile_border);

        ImGui::Checkbox("Hide for depth greater than:", &tile_settings->hide_depth_greater);
        if (tile_settings->hide_depth_greater)
        {
            ImGui::SameLine();
            ImGui::InputInt("##depth", &tile_settings->hide_depth_value);
            limits.hide_depth_value.clamp(tile_settings->hide_depth_value);
        }

        ImGui::Unindent();
    }

    Settings::Solver* solver_settings = settings.get_solver_settings();
    if (solver_settings)
    {
        const auto& limits = settings.read_solver_settings_limits();

        ImVec2 spacing = { 10.f, 10.f };
        ImGui::Dummy(spacing);
        ImGui::BulletText("Solver");
        ImGui::Indent();

        ImGui::Checkbox("Max number of solutions:", &solver_settings->limit_solutions);
        if (solver_settings->limit_solutions)
        {
            ImGui::SameLine();
            ImGui::InputInt("##nb_sols", &solver_settings->max_nb_solutions);
            limits.max_nb_solutions.clamp(solver_settings->max_nb_solutions);
        }

        ImGui::Unindent();
    }

    Settings::Animation* animation_settings = settings.get_animation_settings();
    if (animation_settings)
    {
        const auto& limits = settings.read_animation_settings_limits();

        ImGui::BulletText("Animation");
        ImGui::Indent();

        ImGui::Checkbox("Show branching with colors", &animation_settings->show_branching);

        ImGui::TextUnformatted("Speed:");

        // FTL (Faster Than Light)
        ImGui::SameLine();
        ImGui::Checkbox("FTL", &animation_settings->ftl);

        if (!animation_settings->ftl)
        {
            // Speed slider
            ImGui::SameLine();
            ImGui::SliderInt("##speed", &animation_settings->speed, limits.speed.min, limits.speed.max, "%d", ImGuiSliderFlags_AlwaysClamp);
            assert(animation_settings->speed >= 0);

            // Modifying the speed while in pause will unpause
            if (animation_settings->speed > 0) { animation.paused = false; }
        }

        // Enabling FTL while in pause will unpause
        if (animation_settings->ftl) { animation.paused = false; }

        // Pause button
        if (!animation.paused && ImGui::Button("Pause"))
        {
            animation.paused = true;
            animation.ftl = animation_settings->ftl;
            animation.last_speed = animation_settings->speed;
            animation_settings->ftl = false;
            animation_settings->speed = 0;
        }
        else if (animation.paused && ImGui::Button("Resume"))
        {
            animation.paused = false;
            animation_settings->ftl = animation.ftl;
            animation_settings->speed = animation.last_speed;
        }

        ImGui::Unindent();
    }

    ImGui::End();
}
