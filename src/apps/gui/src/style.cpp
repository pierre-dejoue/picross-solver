#include "style.h"

#include <imgui_wrap.h>


namespace {

constexpr ImColor WindowBackgroundColor_Classic     ( 29,  75,  99, 217);
constexpr ImColor WindowBackgroundColor_Dark        (  4,   8,  25, 240);
constexpr ImColor WindowMainBackgroundColor_Classic ( 35,  92, 121, 255);
constexpr ImColor WindowMainBackgroundColor_Dark    ( 10,  25,  50, 255);
constexpr ImColor LogoBorderColor                   (114, 102,  70, 255);

}

void imgui_set_style(bool dark_mode)
{
    if (dark_mode)
    {
        ImGui::StyleColorsDark();
        ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = WindowBackgroundColor_Dark;
    }
    else
    {
        ImGui::StyleColorsClassic();
        ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = WindowBackgroundColor_Classic;
    }
}

namespace gui_style {

std::array<float, 4> get_background_color(bool dark_mode)
{
    const auto c = static_cast<ImVec4>(dark_mode ? WindowMainBackgroundColor_Dark : WindowMainBackgroundColor_Classic);
    return std::array<float, 4>{ c.x, c.y, c.z, c.w };
}

std::array<float, 4> get_logo_border_color()
{
    static const auto c = static_cast<ImVec4>(LogoBorderColor);
    return std::array<float, 4>{ c.x, c.y, c.z, c.w };
}

} // namespace gui_style
