#pragma once

#include <picross/picross.h>
#include <picross/picross_io.h>

#include <string>

void save_picross_file(const std::string& filepath, const picross::InputGrid& grid, const picross::io::ErrorHandler& error_handler) noexcept;
