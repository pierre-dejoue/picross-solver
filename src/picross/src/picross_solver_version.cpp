/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#include <picross/picross.h>

#include "picross_solver_version.h"

namespace picross {

std::string_view get_version_string()
{
    return PICROSS_SOLVER_LIBRARY_VERSION_STRING;
}

} // namespace picross
