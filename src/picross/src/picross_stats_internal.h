/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include <picross/picross_stats.h>

#include <cassert>


namespace picross
{

void merge_branching_grid_stats(GridStats& stats, const GridStats& branching_stats);

} // namespace picross
