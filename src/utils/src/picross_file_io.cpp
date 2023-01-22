#include <utils/picross_file_io.h>

#include <utils/bitmap_io.h>
#include <utils/strings.h>
#include <utils/text_io.h>

#include <cassert>
#include <exception>
#include <fstream>
#include <sstream>


namespace picross
{
namespace io
{

std::ostream& operator<<(std::ostream& ostream, PicrossFileFormat format)
{
    switch(format)
    {
    case PicrossFileFormat::Native:
        ostream << "Native";
        break;
    case PicrossFileFormat::NIN:
        ostream << "NIN";
        break;
    case PicrossFileFormat::NON:
        ostream << "NON";
        break;
    case PicrossFileFormat::PBM:
        ostream << "PBM";
        break;
    case PicrossFileFormat::OutputGrid:
        ostream << "OutputGrid";
        break;
    default:
        assert(0);
        ostream << "Unknown";
        break;
    }
    return ostream;
}

PicrossFileFormat picross_file_format_from_filepath(std::string_view filepath)
{
    const std::string ext = str_tolower(file_extension(filepath));
    if (ext == "nin")
    {
        return PicrossFileFormat::NIN;
    }
    else if (ext == "non")
    {
        return PicrossFileFormat::NON;
    }
    else if (ext == "pbm")
    {
        return PicrossFileFormat::PBM;
    }
    else
    {
        // Default
        return PicrossFileFormat::Native;
    }
}

std::vector<InputGrid> parse_picross_file(std::string_view filepath, PicrossFileFormat format, const ErrorHandler& error_handler) noexcept
{
    try
    {
        switch(format)
        {
        case PicrossFileFormat::Native:
            return picross::io::parse_input_file_native(filepath, error_handler);

        case PicrossFileFormat::NIN:
            return picross::io::parse_input_file_nin_format(filepath, error_handler);

        case PicrossFileFormat::NON:
            return picross::io::parse_input_file_non_format(filepath, error_handler);

        case PicrossFileFormat::PBM:
        {
            const auto output_grid = import_bitmap_pbm(std::string(filepath), error_handler);
            return { picross::get_input_grid_from(output_grid) };
        }

        case PicrossFileFormat::OutputGrid:
        {
            const auto output_grid = picross::io::parse_output_grid_from_file(filepath, error_handler);
            return { picross::get_input_grid_from(output_grid) };
        }

        default:
            assert(0);
            break;
        }
    }
    catch (const std::exception& e)
    {
        error_handler(e.what(), 1);
    }
    return {};
}

namespace
{
    std::string goal_non_set_error_msg(std::string_view filepath, PicrossFileFormat format)
    {
        std::stringstream msg;
        msg << "Writing file " << filepath << " with format " << format << " requires a goal output grid.";
        return msg.str();
    }
}

void save_picross_file(std::string_view filepath, PicrossFileFormat format, const picross::InputGrid& input_grid, std::optional<OutputGrid> goal, const picross::io::ErrorHandler& error_handler) noexcept
{
    try
    {
        switch(format)
        {
        case PicrossFileFormat::Native:
        {
            std::ofstream out(filepath.data());
            if (!out.good())
                error_handler("Error writing file " + std::string(filepath), 1);
            picross::io::write_input_grid_native(out, input_grid);
            break;
        }

        case PicrossFileFormat::NIN:
        {
            std::ofstream out(filepath.data());
            if (!out.good())
                error_handler("Error writing file " + std::string(filepath), 1);
            picross::io::write_input_grid_nin_format(out, input_grid);
            break;
        }

        case PicrossFileFormat::NON:
        {
            std::ofstream out(filepath.data());
            if (!out.good())
                error_handler("Error writing file " + std::string(filepath), 1);
            picross::io::write_input_grid_non_format(out, input_grid);
            break;
        }

        case PicrossFileFormat::PBM:
        {
            if (!goal)
                error_handler(goal_non_set_error_msg(filepath, format), 2);
            export_bitmap_pbm(std::string(filepath), *goal, error_handler);
            break;
        }

        case PicrossFileFormat::OutputGrid:
        {
            if (!goal)
                error_handler(goal_non_set_error_msg(filepath, format), 2);
            std::ofstream out(filepath.data());
            if (!out.good())
                error_handler("Error writing file " + std::string(filepath), 1);
            out << *goal;
            break;
        }

        default:
            assert(0);
            break;
        }
    }
    catch (const std::exception& e)
    {
        error_handler(e.what(), 3);
    }
}

} // namespace io
} // namespace picross
