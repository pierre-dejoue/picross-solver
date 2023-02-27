#include "draw_grid.h"

#include <cassert>
#include <vector>


namespace picross_grid
{
namespace
{
    // Grid background
    constexpr ImU32 ColorGridBack = IM_COL32(255, 255, 255, 255);
    constexpr ImU32 ColorGridOutline = IM_COL32(224, 224, 224, 255);
    constexpr ImU32 ColorGridOutlineFive = IM_COL32(178, 178, 178, 255);

    // Default tile colors (branching depth = 0)
    constexpr ImU32 ColorTileBorder = IM_COL32(20, 90, 116, 255);
    constexpr ImU32 ColorTileFilled = IM_COL32(91, 94, 137, 255);
    constexpr ImU32 ColorTileEmpty = IM_COL32(212, 216, 224, 255);

    // Branching colors
    constexpr ImU32 ColorTileDepth1Border = IM_COL32(116, 20, 90, 255);
    constexpr ImU32 ColorTileDepth1Filled = IM_COL32(137, 91, 94, 255);
    constexpr ImU32 ColorTileDepth1Empty = IM_COL32(224, 212, 216, 255);

    constexpr ImU32 ColorTileDepth2Border = IM_COL32(90, 116, 20, 255);
    constexpr ImU32 ColorTileDepth2Filled = IM_COL32(94, 137, 91, 255);
    constexpr ImU32 ColorTileDepth2Empty = IM_COL32(216, 224, 212, 255);

    // Branching colors cont. (periodical from there)
    constexpr ImU32 ColorTileDepthCyc0Border = IM_COL32(114, 52, 20, 255);
    constexpr ImU32 ColorTileDepthCyc0Filled = IM_COL32(135, 135, 90, 255);
    constexpr ImU32 ColorTileDepthCyc0Empty = IM_COL32(224, 224, 216, 255);

    constexpr ImU32 ColorTileDepthCyc1Border = IM_COL32(52, 20, 114, 255);
    constexpr ImU32 ColorTileDepthCyc1Filled = IM_COL32(135, 90, 135, 255);
    constexpr ImU32 ColorTileDepthCyc1Empty = IM_COL32(224, 216, 224, 255);

    constexpr ImU32 ColorTileDepthCyc2Border = IM_COL32(20, 114, 54, 255);
    constexpr ImU32 ColorTileDepthCyc2Filled = IM_COL32(90, 135, 135, 255);
    constexpr ImU32 ColorTileDepthCyc2Empty = IM_COL32(216, 224, 224, 255);

    // Hidden tile colors (branching depth = 0)
    constexpr ImU32 ColorHiddenTileBorder = IM_COL32(180, 180, 230, 255);
    constexpr ImU32 ColorHiddenTileFilled = IM_COL32(220, 220, 240, 255);
    constexpr ImU32 ColorHiddenTileEmpty = IM_COL32(245, 245, 250, 255);

    unsigned int cycling_color_index(unsigned int depth)
    {
        return depth < 3
            ? depth
            : 3 + ((depth - 3) % 3);
    }

    const ImU32& get_color_tile_border(unsigned int depth = 0)
    {
        static const ImU32 Colors[] = {
            ColorTileBorder,
            ColorTileDepth1Border,
            ColorTileDepth2Border,
            ColorTileDepthCyc0Border,
            ColorTileDepthCyc1Border,
            ColorTileDepthCyc2Border
        };
        return Colors[cycling_color_index(depth)];
    }

    const ImU32& get_color_tile_filled(unsigned int depth = 0)
    {
        static const ImU32 Colors[] = {
            ColorTileFilled,
            ColorTileDepth1Filled,
            ColorTileDepth2Filled,
            ColorTileDepthCyc0Filled,
            ColorTileDepthCyc1Filled,
            ColorTileDepthCyc2Filled
         };
        return Colors[cycling_color_index(depth)];
    }

    const ImU32& get_color_tile_empty(unsigned int depth = 0)
    {
        static const ImU32 Colors[] = {
            ColorTileEmpty,
            ColorTileDepth1Empty,
            ColorTileDepth2Empty,
            ColorTileDepthCyc0Empty,
            ColorTileDepthCyc1Empty,
            ColorTileDepthCyc2Empty
        };
        return Colors[cycling_color_index(depth)];
    }

    size_t background_grid_thickness(size_t tile_size)
    {
        // Note that tile_size includes the grid thickness if the grid outline is visible
        return tile_size > 3 ? 1 : 0;
    }

    ImU32 outline_color(size_t idx, bool five_tile_outline = false)
    {
        return (five_tile_outline && idx % 5 == 0) ? ColorGridOutlineFive : ColorGridOutline;
    }

    void draw_tile(ImDrawList* draw_list, ImVec2 tl_corner, size_t tile_size, size_t i, size_t j, bool filled, bool hidden, unsigned int color_depth = 0, float size_ratio = 1.f, float rounding_ratio = 0.f)
    {
        const bool draw_border = tile_size > 6;
        const auto grid_thickness = background_grid_thickness(tile_size);

        assert(0.f < size_ratio && size_ratio <= 1.f);
        assert(0.f <= rounding_ratio && rounding_ratio <= 1.f);
        const float padding = draw_border ? static_cast<float>(tile_size - 1) * 0.5f * (1.f - size_ratio) : 0.f;
        const float rounding = draw_border ? static_cast<float>(tile_size - 1) * 0.5f * rounding_ratio : 0.f;
        const ImVec2 tl_tile_corner = ImVec2(
            tl_corner.x + static_cast<float>(i * tile_size + grid_thickness) + padding,
            tl_corner.y + static_cast<float>(j * tile_size + grid_thickness) + padding);
        const ImVec2 br_tile_corner = ImVec2(
            tl_corner.x + static_cast<float>((i+1) * tile_size) - padding,
            tl_corner.y + static_cast<float>((j+1) * tile_size) - padding);
        const ImU32 tile_color = hidden
            ? (filled ? ColorHiddenTileFilled : ColorHiddenTileEmpty)
            : (filled ? get_color_tile_filled(color_depth) : get_color_tile_empty(color_depth));
        draw_list->AddRectFilled(tl_tile_corner, br_tile_corner, tile_color, rounding);
        if (draw_border)
        {
            const ImU32 border_color = hidden ? ColorHiddenTileBorder : get_color_tile_border(color_depth);
            draw_list->AddRect(tl_tile_corner, br_tile_corner, border_color, rounding);
        }
    }

} // Anonymous namespace


size_t get_tile_size(int size_enum)
{
    // The tile size includes the thickness of the grid (1 pixel), unless for values smaller than 4 in which case the tiles are in contact with each other
    static const std::vector<size_t> TileSizes = { 2, 5, 9, 12, 16 };
    return TileSizes.at(static_cast<size_t>(size_enum));
}

void draw_background_grid(ImDrawList* draw_list, ImVec2 tl_corner, size_t tile_size, size_t width, size_t height, bool outline, bool five_tile_outline)
{
    assert(draw_list);
    const ImVec2 br_corner = ImVec2(tl_corner.x + static_cast<float>(width * tile_size), tl_corner.y + static_cast<float>(height * tile_size));
    draw_list->AddRectFilled(tl_corner, br_corner, ColorGridBack);
    if (background_grid_thickness(tile_size) > 0 && outline)
    {
        for (size_t i = 0u; i <= width; ++i)
        {
            const float x = static_cast<float>(i * tile_size);
            draw_list->AddLine(ImVec2(tl_corner.x + x, tl_corner.y), ImVec2(tl_corner.x + x, br_corner.y), outline_color(i, five_tile_outline));
        }
        for (size_t j = 0u; j <= height; ++j)
        {
            const float y = static_cast<float>(j * tile_size);
            draw_list->AddLine(ImVec2(tl_corner.x, tl_corner.y + y), ImVec2(br_corner.x, tl_corner.y + y), outline_color(j, five_tile_outline));
        }
        draw_list->AddRect(tl_corner, ImVec2(br_corner.x + 1.f, br_corner.y + 1.f), outline_color(0));
    }
}

void draw_grid(ImDrawList* draw_list, ImVec2 tl_corner, size_t tile_size, const ObserverGrid& grid, const Settings::Tile& tile_settings, bool color_by_depth)
{
    assert(draw_list);
    const size_t width = grid.width();
    const size_t height = grid.height();
    for (size_t i = 0u; i < width; ++i)
    {
        for (size_t j = 0u; j < height; ++j)
        {
            const auto tile = grid.get_tile(i, j);
            if (tile == picross::Tile::UNKNOWN)
                continue;
            const auto depth = grid.get_depth(i, j);
            const auto color_depth = color_by_depth ? depth : 0;
            const bool filled = (tile == picross::Tile::FILLED);
            const bool hidden = tile_settings.hide_depth_greater ? (depth >= tile_settings.hide_depth_value) : false;
            draw_tile(draw_list, tl_corner, tile_size, i, j, filled, hidden, color_depth, tile_settings.size_ratio, tile_settings.rounding_ratio);
        }
    }
}

} // namespace picross_grid
