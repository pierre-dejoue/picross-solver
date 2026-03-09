// Copyright (c) 2024 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#pragma once

#include <algorithm>
#include <array>

namespace stdutils {

/**
 * ColorData
 *
 * Floating-point (0.f-1.f) encoding of a color
 */
using ColorData = std::array<float, 4>;               // r,   g,   b,   a

static constexpr ColorData COLOR_DATA_BLACK           {0.f, 0.f, 0.f, 1.f};
static constexpr ColorData COLOR_DATA_WHITE           {1.f, 1.f, 1.f, 1.f};
static constexpr ColorData COLOR_DATA_RED             {1.f, 0.f, 0.f, 1.f};
static constexpr ColorData COLOR_DATA_TRANSPARENT     {0.f, 0.f, 0.f, 1.f};

inline constexpr ColorData COLOR_DATA_GRAY(float lum) { lum = std::clamp(lum, 0.f, 1.f); return ColorData{ lum, lum, lum, 1.f}; }

inline float brightness(const ColorData& col) { return (col[0] + col[1] + col[2]) / 3.f; }

} // namespace stdutils
