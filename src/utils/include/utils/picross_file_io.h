#pragma once

#include <picross/picross.h>

#include <optional>
#include <ostream>
#include <string_view>

namespace picross
{
namespace io
{

enum class PicrossFileFormat
{
    Native,
    NON,
    PBM,
    OutputGrid
};

std::ostream& operator<<(std::ostream& ostream, PicrossFileFormat format);

PicrossFileFormat picross_file_format_from_filepath(std::string_view filepath);

std::vector<InputGrid> parse_picross_file(std::string_view filepath, PicrossFileFormat format, const ErrorHandler& error_handler) noexcept;

void save_picross_file(std::string_view filepath, PicrossFileFormat format, const InputGrid& input_grid, std::optional<OutputGrid> goal, const ErrorHandler& error_handler) noexcept;

}
}
