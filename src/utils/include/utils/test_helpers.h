#pragma once

#include <picross/picross.h>

#include <cstddef>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace picross
{
// Utility functions to build an OutputGrid from a vector of tiles, or a string, arranged in row-major order:

// The input is a vector<int>:
//   0: the tile is empty
//   1: the tile is filled
OutputGrid build_output_grid_from(std::size_t width, std::size_t height, const std::vector<int>& tiles, std::string_view name = "Test");

// The input is a string:
//  '0' or '.': the tile is empty
//  '1' or '#': the tile is filled
// (All other characters are ignored)
OutputGrid build_output_grid_from(std::size_t width, std::size_t height, std::string_view tiles, std::string_view name = "Test");


// Hasher for OutputGrid
struct OutputGridHasher
{
    std::size_t operator()(const OutputGrid& grid) const noexcept;
};

using OutputGridSet = std::unordered_set<OutputGrid, OutputGridHasher>;
} // namesapce picross