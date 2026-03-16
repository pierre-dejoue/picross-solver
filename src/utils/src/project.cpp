#include <utils/project.h>

#include "project_defines.h"

namespace project {

std::string_view get_name()
{
    return THIS_PROJECT_NAME;
}

std::string_view get_short_license()
{
    return THIS_PROJECT_SHORT_LICENSE;
}

std::string_view get_short_copyright()
{
    return THIS_PROJECT_SHORT_COPYRIGHT;
}

std::string_view get_website()
{
    return "https://github.com/pierre-dejoue/picross-solver";
}

std::string_view get_compilation_target_platform()
{
    return THIS_PROJECT_TARGET_PLATFORM;
}

} // namespace project
