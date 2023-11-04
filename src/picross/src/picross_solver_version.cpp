/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#include <picross/picross.h>

#include "picross_solver_version.h"


namespace picross
{

std::string_view get_version_string()
{
    return std::string_view(PICROSS_SOLVER_VERSION_STRING);
}

} // namespace picross
