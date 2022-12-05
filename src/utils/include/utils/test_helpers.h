#pragma once

#include <picross/picross.h>

// Build a output grid from a vector of tiles in row-major order
//   0: the tile is empty
//   1: the tile is filled
picross::OutputGrid build_output_grid_from(std::size_t width, std::size_t height, const std::vector<int>& tiles);
