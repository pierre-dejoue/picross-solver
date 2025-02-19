// Copyright (c) 2023 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#include <stdutils/io.h>

#include <array>
#include <cassert>

namespace stdutils {
namespace io {

namespace {

constexpr unsigned int SEV_CODE_MAX_IDX = 7;
std::array<std::string_view, SEV_CODE_MAX_IDX + 1> str_severity_code_lookup {
    "FATAL",
    "EXCPT",
    "ZERO",
    "ERROR",
    "WARNING",
    "INFO",
    "TRACE",
    "UNKNOWN"
};

} // namespace

std::string_view str_severity_code(SeverityCode code) noexcept
{
    unsigned int code_idx = static_cast<unsigned int>(code - Severity::FATAL);
    if (code_idx > SEV_CODE_MAX_IDX) { code_idx = SEV_CODE_MAX_IDX; }
    assert(code_idx < str_severity_code_lookup.size());
    return str_severity_code_lookup[code_idx];
}

} // namespace io
} // namespace stdutils
