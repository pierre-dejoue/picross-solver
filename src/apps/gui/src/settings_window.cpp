#include "settings_window.h"

#include <imgui_wrap.h>

#include "imgui_helpers.h"
#include "settings.h"
#include "window_layout.h"

#include <cassert>

SettingsWindow::SettingsWindow(Settings& settings)
    : settings(settings)
    , title("Settings")
    , animation()
{
}

void SettingsWindow::visit(bool& can_be_erased)
{
    can_be_erased = false;        // Cannot close

    const ImVec2 top_right_corner = [](const float padding) {
        const ImVec2& work_pos = ImGui::GetMainViewport()->WorkPos;
        const ImVec2& work_sz  = ImGui::GetMainViewport()->WorkSize;
        return ImVec2(work_pos.x + work_sz.x - padding, work_pos.y + padding);
    }(WindowLayout::DEFAULT_PADDING);
    ImGui::SetNextWindowPos(top_right_corner, ImGuiCond_Once, ImVec2(1.f, 0.f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 300), ImVec2(FLT_MAX, 600));
    constexpr ImGuiWindowFlags win_flags = ImGuiWindowFlags_AlwaysAutoResize
                                         | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin(title.c_str(), nullptr, win_flags))
    {
        // Collapsed
        ImGui::End();
        return;
    }

    Settings::UI* ui_settings = settings.get_ui_settings();
    if (ui_settings)
    {
        //const auto& limits = settings.read_ui_settings_limits();

        ImGui::BulletText("UI");
        ImGui::Indent();

        int extra_scaling = static_cast<int>(ui_settings->extra_scaling);
        bool pressed = false;
        ImGui::Text("Scaling: ");
        ImGui::SameLine(); pressed  = ImGui::RadioButton("Small##UI", &extra_scaling, 0);
        ImGui::SameLine(); pressed |= ImGui::RadioButton("Mid##UI",   &extra_scaling, 1);
        ImGui::SameLine(); pressed |= ImGui::RadioButton("Big##UI",   &extra_scaling, 2);
        if (pressed)
        {
            ui_settings->extra_scaling = static_cast<gui_style::ExtraScaling>(extra_scaling);
        }

        ImGui::Unindent();
    }

    Settings::Tile* tile_settings = settings.get_tile_settings();
    if (tile_settings)
    {
        const auto& limits = settings.read_tile_settings_limits();

        ImGui::BulletText("Tiles");
        ImGui::Indent();

        ImGui::Text("Size: ");
        ImGui::SameLine(); ImGui::RadioButton("XS", &tile_settings->size_enum, 0);
        ImGui::SameLine(); ImGui::RadioButton("S",  &tile_settings->size_enum, 1);
        ImGui::SameLine(); ImGui::RadioButton("M",  &tile_settings->size_enum, 2);
        ImGui::SameLine(); ImGui::RadioButton("L",  &tile_settings->size_enum, 3);
        ImGui::SameLine(); ImGui::RadioButton("XL", &tile_settings->size_enum, 4);

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

        // Always reset the "one step" flag between frames
        animation_settings->one_event = false;

        ImGui::TextUnformatted("Speed:");

        // FTL (Faster Than Light)
        ImGui::SameLine();
        ImGui::Checkbox("FTL", &animation_settings->ftl);
        ImGui::HelpMarker("Faster Than Light. Solves the puzzle as fast as possible.");

        if (!animation_settings->ftl)
        {
            // Speed slider
            ImGui::SameLine();
            ImGui::SliderInt("##speed", &animation_settings->speed, limits.speed.min, limits.speed.max, "%d", ImGuiSliderFlags_AlwaysClamp);
            ImGui::TooltipTextUnformatted("Set a upper limit to the solving speed. Set to zero will pause the current solve.");
            assert(animation_settings->speed >= 0);

            // Modifying the speed while in pause will unpause
            if (animation_settings->speed > 0) { animation.paused = false; }
        }

        // Enabling FTL while in pause will unpause
        if (animation_settings->ftl) { animation.paused = false; }

        // The next three buttons are on the same line
        bool same_line = false;

        // Pause button
        if (!animation.paused && animation_settings->speed > 0)
        {
            same_line = true;
            if (ImGui::Button("Pause"))
            {
                animation.paused = true;
                animation.ftl = animation_settings->ftl;
                animation.last_speed = animation_settings->speed;
                animation_settings->ftl = false;
                animation_settings->speed = 0;
            }
        }

        // Resume button
        if (animation.paused && animation.last_speed > 0)
        {
            if (same_line) { ImGui::SameLine(); }
            same_line = true;
            if (ImGui::Button("Resume"))
            {
                animation.paused = false;
                animation_settings->ftl = animation.ftl;
                animation_settings->speed = animation.last_speed;
                animation.last_speed = 0;
            }
        }

        // One-step button
        if (animation.paused || animation_settings->speed == 0)
        {
            if (same_line) { ImGui::SameLine(); }
            same_line = true;
            if (ImGui::Button("One step"))
            {
                animation_settings->one_event = true;
            }
        }

        ImGui::Unindent();
    }

    ImGui::End();
}
