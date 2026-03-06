// Copyright (c) 2026 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#pragma once

#include <string_view>

namespace project {

// Get the project name
std::string_view get_name();

// Get the short license description (e.g. "MIT License", or "All rights reserved", etc.)
std::string_view get_short_license();

// Get the short copyright notice (Copyright (c) <year> <owner>)
std::string_view get_short_copyright();

// Get the project website
std::string_view get_website();

} // namespace project
