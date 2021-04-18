#include "settings_window.h"

#include "settings.h"

SettingsWindow::SettingsWindow(Settings& settings)
    : settings(settings)
    , title("Settings")
{
}

void SettingsWindow::visit(bool& canBeErased)
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 300), ImVec2(FLT_MAX, 600));

    bool isWindowOpen;
    if (!ImGui::Begin(title.c_str(), &isWindowOpen, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // Collapsed
        canBeErased = !isWindowOpen;
        ImGui::End();
        return;
    }
    canBeErased = false;        // Cannot close

    Settings::Tile* tile_settings = settings.get_tile_settings();
    if (tile_settings)
    {
        const auto& limits = settings.read_tile_settings_limits();

        ImGui::BulletText("Tile settings");
        ImGui::Indent();

        ImGui::Text("Size: ");
        ImGui::SameLine();
        ImGui::RadioButton("small", &tile_settings->size_enum, 0);
        ImGui::SameLine();
        ImGui::RadioButton("medium", &tile_settings->size_enum, 1);
        ImGui::SameLine();
        ImGui::RadioButton("large", &tile_settings->size_enum, 2);

        ImGui::SliderFloat("Rounding ratio", &tile_settings->rounding_ratio, limits.rounding_ratio.min, limits.rounding_ratio.max, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::SliderFloat("Size ratio", &tile_settings->size_ratio, limits.size_ratio.min, limits.size_ratio.max, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Unindent();
    }

    ImGui::End();
}
