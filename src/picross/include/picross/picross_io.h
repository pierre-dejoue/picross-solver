/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PUBLIC API of the Picross solver
 *
 *   - File IO
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include "picross_input_grid.h"

#include <functional>
#include <ostream>
#include <string_view>
#include <vector>


namespace picross
{
namespace io
{

using ExitCode = int;
using ErrorHandler = std::function<void(std::string_view, ExitCode)>;

/*
 * File parser, native file format
 *
 *      # comment           <--- lines starting with # are ignored
 *      GRID name           <--- marker for the beginning of a new grid
 *      ROWS                <--- marker to start listing the constraints on the rows
 *      [ 1 2 3 ]           <--- constraint on one line (here a row)
 *      ...
 *      COLUMNS             <--- marker for columns
 *      [ ]                 <--- an empty constraint
 *      ...
 *
 * - Multiple grids can be declared in a single file
 * - The GRID marker is mandatory before the ROWS and COLUMNS sections, however the grid name is optional
 * - ROWS and COLUMNS sections are independent and can be in any order
 * - Empty lines are skipped
 *
 */
std::vector<InputGrid> parse_input_file_native(std::string_view filepath, const ErrorHandler& error_handler) noexcept;

/*
 * File parser, NIN file format used by Jakub Wilk's nonogram solver program
 *
 *      10 10               <--- Width and height
 *      1 2 3               <--- Constraint on one line, first rows then columns
 *      ...
 *
 * Example of puzzles with this format: https://github.com/jwilk-archive/nonogram/tree/master/data
 */
std::vector<InputGrid> parse_input_file_nin_format(std::string_view filepath, const ErrorHandler& error_handler) noexcept;

/*
 * File parser, NON file format (originally by Steve Simpson)
 *
 *   https://github.com/mikix/nonogram-db/blob/master/FORMAT.md
 *
 */
std::vector<InputGrid> parse_input_file_non_format(std::string_view filepath, const ErrorHandler& error_handler) noexcept;

/*
 * Stream writer, native file format
 */
void write_input_grid_native(std::ostream& ostream, const InputGrid& input_grid);

/*
 * Stream writer, NIN file format
 */
void write_input_grid_nin_format(std::ostream& ostream, const InputGrid& input_grid);

/*
 * Stream writer, NON file format
 */
void write_input_grid_non_format(std::ostream& ostream, const InputGrid& input_grid);

} // namespace io
} // namespace picross
