#pragma once

#include "settings.h"

#include <picross/picross.h>
#include <utils/grid_observer.h>

#include <imgui_wrap.h>

#include <cstddef>

namespace picross_grid {

size_t get_tile_size(int size_enum);
void draw_background_grid(ImDrawList* draw_list, ImVec2 tl_corner, size_t tile_size, size_t width, size_t height, bool outline = false, bool five_tile_outline = false);
void draw_grid(ImDrawList* draw_list, ImVec2 tl_corner, size_t tile_size, const ObserverGrid& grid, const Settings::Tile& tile_settings, bool color_by_depth = false);

}