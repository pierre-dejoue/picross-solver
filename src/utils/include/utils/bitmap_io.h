#pragma once

#include <picross/picross.h>

#include <string>
#include <memory>

picross::OutputGrid import_bitmap_pbm(const std::string& filepath, const picross::io::ErrorHandler& error_handler) noexcept;

void export_bitmap_pbm(const std::string& filepath, const picross::OutputGrid& grid, const picross::io::ErrorHandler& error_handler) noexcept;
