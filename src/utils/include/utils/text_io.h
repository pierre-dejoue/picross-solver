#pragma once

#include <picross/picross.h>

#include <cstddef>
#include <string_view>
#include <vector>

namespace picross {

// Utility functions to build an OutputGrid from a vector of tiles, or a string, arranged in row-major order:

// The input is a vector<int>:
//  <0: the tile is unknown
//   0: the tile is empty
//  >0: the tile is filled
OutputGrid build_output_grid_from(std::size_t width, std::size_t height, const std::vector<int>& tiles, std::string_view name = "Test");

// The input is a string:
//  '?' :       the tile is unknown
//  '0' or '.': the tile is empty
//  '1' or '#': the tile is filled
// (All other characters are ignored)
// Unless the grid width and height are given as arguments, they are deduced from the presence of EOL characters
OutputGrid build_output_grid_from(std::size_t width, std::size_t height, std::string_view tiles, std::string_view name = "Test");
OutputGrid build_output_grid_from(std::string_view tiles, std::string_view name = "Test");
namespace io {
OutputGrid parse_output_grid_from_file(std::string_view filepath, const io::ErrorHandler& error_handler) noexcept;
}

// Utility function to build a Line from a string representing the tiles:
// '.': the tile is empty
// '#': the tile is filled
// '?': the tile is unknown
// All other characters throw
Line build_line_from(std::string_view tiles, Line::Type type = Line::ROW, unsigned int index = 0);
Line build_line_from(char tile, std::size_t count, Line::Type type = Line::ROW, unsigned int index = 0);

} // namespace picross
