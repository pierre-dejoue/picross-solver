#pragma once

#include <picross/picross.h>
#include <picross/picross_io.h>

#include <string>
#include <memory>


std::unique_ptr<picross::OutputGrid> import_bitmap_pbm(const std::string& filepath, const picross::ErrorHandler& error_handler) noexcept;

void export_bitmap_pbm(const std::string& filepath, const picross::OutputGrid& grid, const picross::ErrorHandler& error_handler) noexcept;
