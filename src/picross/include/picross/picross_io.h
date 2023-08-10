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
#include "picross_output_grid.h"

#include <functional>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>


namespace picross
{

/*
 * IOGrid class
 *
 * A puzzle grid for IO purposes. It comprises of:
 *  - An InputGrid: The definition of the puzzle
 *  - An optional OutputGrid: The goal (solution) of the puzzle, it is known
 */
struct IOGrid
{
    IOGrid(const InputGrid& input_grid, const std::optional<OutputGrid>& goal = std::nullopt);
    IOGrid(InputGrid&& input_grid, std::optional<OutputGrid>&& goal = std::nullopt) noexcept;

    InputGrid                 m_input_grid;
    std::optional<OutputGrid> m_goal;
};

namespace io
{

/*
 * IO error handling
 */
using ErrorCodeT = int;
struct ErrorCode
{
    // All codes != 0
    static constexpr ErrorCodeT PARSING_ERROR = 1;
    static constexpr ErrorCodeT FILE_ERROR = 2;
    static constexpr ErrorCodeT EXCEPTION = 3;
    static constexpr ErrorCodeT WARNING = 4;
};

std::string str_error_code(ErrorCodeT code);

using ErrorHandler = std::function<void(ErrorCodeT, std::string_view)>;

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
std::vector<IOGrid> parse_input_file_native(std::string_view filepath, const ErrorHandler& error_handler) noexcept;

/*
 * File parser, NIN file format used by Jakub Wilk's nonogram solver program
 *
 *      10 10               <--- Width and height
 *      1 2 3               <--- Constraint on one line, first rows then columns
 *      ...
 *
 * Example of puzzles with this format: https://github.com/jwilk-archive/nonogram/tree/master/data
 */
std::vector<IOGrid> parse_input_file_nin_format(std::string_view filepath, const ErrorHandler& error_handler) noexcept;

/*
 * File parser, NON file format (originally by Steve Simpson)
 *
 *   https://github.com/mikix/nonogram-db/blob/master/FORMAT.md
 *
 */
std::vector<IOGrid> parse_input_file_non_format(std::string_view filepath, const ErrorHandler& error_handler) noexcept;

/*
 * Stream writer, native file format
 */
void write_input_grid_native(std::ostream& out, const IOGrid& grid);

/*
 * Stream writer, NIN file format
 */
void write_input_grid_nin_format(std::ostream& out, const IOGrid& grid);

/*
 * Stream writer, NON file format
 */
void write_input_grid_non_format(std::ostream& out, const IOGrid& grid);

} // namespace io
} // namespace picross
