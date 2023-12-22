#include <utils/picross_file_io.h>

#include <stdutils/string.h>
#include <utils/bitmap_io.h>
#include <utils/text_io.h>

#include <cassert>
#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>


namespace picross
{
namespace io
{

std::ostream& operator<<(std::ostream& out, PicrossFileFormat format)
{
    switch(format)
    {
    case PicrossFileFormat::Native:
        out << "Native";
        break;
    case PicrossFileFormat::NIN:
        out << "NIN";
        break;
    case PicrossFileFormat::NON:
        out << "NON";
        break;
    case PicrossFileFormat::PBM:
        out << "PBM";
        break;
    case PicrossFileFormat::OutputGrid:
        out << "OutputGrid";
        break;
    default:
        assert(0);
        out << "Unknown";
        break;
    }
    return out;
}

PicrossFileFormat picross_file_format_from_filepath(std::string_view filepath)
{
    std::string ext = stdutils::string::tolower(
        std::filesystem::path(filepath).extension().string()
    );
    if (ext == ".nin")
    {
        return PicrossFileFormat::NIN;
    }
    else if (ext == ".non")
    {
        return PicrossFileFormat::NON;
    }
    else if (ext == ".pbm")
    {
        return PicrossFileFormat::PBM;
    }
    else
    {
        // Default
        return PicrossFileFormat::Native;
    }
}

std::vector<IOGrid> parse_picross_file(std::string_view filepath, PicrossFileFormat format, const ErrorHandler& error_handler) noexcept
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
            auto goal = import_bitmap_pbm(std::string(filepath), error_handler);
            auto input_grid = get_input_grid_from(goal);
            return { IOGrid(std::move(input_grid), std::make_optional<OutputGrid>(std::move(goal))) };
        }

        case PicrossFileFormat::OutputGrid:
        {
            auto goal = parse_output_grid_from_file(filepath, error_handler);
            auto input_grid = get_input_grid_from(goal);
            return { IOGrid(std::move(input_grid), std::make_optional<OutputGrid>(std::move(goal))) };
        }

        default:
            assert(0);
            break;
        }
    }
    catch (const std::exception& e)
    {
        error_handler(ErrorCode::EXCEPTION, e.what());
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

void save_picross_file(std::string_view filepath, PicrossFileFormat format, const IOGrid& io_grid, const picross::io::ErrorHandler& error_handler) noexcept
{
    try
    {
        switch(format)
        {
        case PicrossFileFormat::Native:
        {
            std::ofstream out(filepath.data());
            if (!out.good())
                error_handler(ErrorCode::FILE_ERROR, "Error writing file " + std::string(filepath));
            picross::io::write_input_grid_native(out, io_grid);
            break;
        }

        case PicrossFileFormat::NIN:
        {
            std::ofstream out(filepath.data());
            if (!out.good())
                error_handler(ErrorCode::FILE_ERROR, "Error writing file " + std::string(filepath));
            picross::io::write_input_grid_nin_format(out, io_grid);
            break;
        }

        case PicrossFileFormat::NON:
        {
            std::ofstream out(filepath.data());
            if (!out.good())
                error_handler(ErrorCode::FILE_ERROR, "Error writing file " + std::string(filepath));
            picross::io::write_input_grid_non_format(out, io_grid);
            break;
        }

        case PicrossFileFormat::PBM:
        {
            if (!io_grid.m_goal)
                error_handler(ErrorCode::WARNING, goal_non_set_error_msg(filepath, format));
            export_bitmap_pbm(std::string(filepath), *io_grid.m_goal, error_handler);
            break;
        }

        case PicrossFileFormat::OutputGrid:
        {
            if (!io_grid.m_goal)
                error_handler(ErrorCode::WARNING, goal_non_set_error_msg(filepath, format));
            std::ofstream out(filepath.data());
            if (!out.good())
                error_handler(ErrorCode::FILE_ERROR, "Error writing file " + std::string(filepath));
            out << *io_grid.m_goal;
            break;
        }

        default:
            assert(0);
            break;
        }
    }
    catch (const std::exception& e)
    {
        error_handler(ErrorCode::EXCEPTION, e.what());
    }
}

} // namespace io
} // namespace picross
