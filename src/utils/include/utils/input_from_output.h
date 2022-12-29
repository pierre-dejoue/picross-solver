#pragma once

#include <picross/picross.h>

namespace picross
{

/*
 * Build an InputGrid from an OutputGrid
 */
InputGrid build_input_grid_from(const OutputGrid& grid);

} // namespace picross
