// Copyright (c) 2023 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#include <stdutils/io.h>

#include <cassert>


namespace stdutils
{
namespace io
{

std::string_view str_severity_code(SeverityCode code)
{
    if (code == Severity::FATAL)
    {
        return "FATAL";
    }
    else if (code == Severity::EXCPT)
    {
        return "EXCPT";
    }
    else if (code == Severity::WARN)
    {
        return "WARNING";
    }
    else if (code == Severity::ERR)
    {
        return "ERROR";
    }
    else
    {
        return "UNKNOWN";
    }
}

}
}
