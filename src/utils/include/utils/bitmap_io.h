#pragma once

#include <picross/picross.h>

#include <string>
#include <memory>

// Read a PBM file (in ascii or binary format)
picross::OutputGrid import_bitmap_pbm(const std::string& filepath, const picross::io::ErrorHandler& error_handler) noexcept;

// Save grid as a binary PBM file
bool export_bitmap_pbm(const std::string& filepath, const picross::OutputGrid& grid, const picross::io::ErrorHandler& error_handler) noexcept;
