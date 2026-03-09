#pragma once

#include <stdutils/color.h>

void imgui_set_style(bool dark_mode);

namespace gui_style {

stdutils::ColorData get_background_color(bool dark_mode);
stdutils::ColorData get_logo_border_color();

constexpr float get_logo_border_size() noexcept { return 1.f; }

} // namespace gui_style
