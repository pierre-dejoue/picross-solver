/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PUBLIC API of the Picross solver
 *
 *   - File IO
 *
 * Copyright (c) 2010-2020 Pierre DEJOUE
 ******************************************************************************/
#pragma once


#include <functional>
#include <string>
#include <vector>


#include <picross/picross.h>


namespace picross
{

using ExitCode = int;
using ErrorHandler = std::function<void(const std::string&, ExitCode)>;


/*******************************************************************************
 * Picross file format:
 *
 *      # comment           <--- lines starting with # are ignored
 *      GRID name           <--- marker for the beginning of a new grid
 *      ROWS                <--- marker to start listing the constraints on the rows
 *      [ 1 2 3 ]           <--- constraint on one line (here a row)
 *      ...
 *      COLUMNS             <--- marker for columns
 *      ...
 *
 * NB: Empty lines are skipped
 * NB: Grid name is optional
 *
 ******************************************************************************/
std::vector<InputGrid> parse_input_file(const std::string& filepath, const ErrorHandler& error_handler) noexcept;

} // namespace picross
