#pragma once

#include <stdutils/color.h>

void imgui_set_style(bool dark_mode);

namespace gui_style {

stdutils::ColorData get_background_color(bool dark_mode);
stdutils::ColorData get_logo_border_color();

constexpr float get_logo_border_size() noexcept { return 1.f; }

enum class ExtraScaling
{
    Small = 0,
    Mid,
    Big,
};

float get_extra_scaling(ExtraScaling extra_scaling);

} // namespace gui_style
