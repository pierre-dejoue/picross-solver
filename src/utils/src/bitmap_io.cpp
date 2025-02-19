#include <utils/bitmap_io.h>

#include <pnm.hpp>

#include <cassert>
#include <cstddef>
#include <exception>
#include <filesystem>

picross::OutputGrid import_bitmap_pbm(const std::string& filepath, const picross::io::ErrorHandler& error_handler) noexcept
{
    try
    {
        const pnm::pbm_image bitmap = pnm::read_pbm(filepath);
        const auto grid_name = std::filesystem::path(filepath).stem().string();
        picross::OutputGrid output_grid(bitmap.width(), bitmap.height(), picross::Tile::UNKNOWN, grid_name);
        std::size_t y = 0u;
        for (const auto& line : bitmap.lines())
        {
            std::size_t x = 0u;
            for (const auto& pix : line)
            {
                output_grid.set_tile(x++, y, pix.value ? picross::Tile::FILLED : picross::Tile::EMPTY);
            }
            y++;
        }
        return output_grid;
    }
    catch(const std::exception& e)
    {
        error_handler(picross::io::ErrorCode::EXCEPTION, e.what());
    }
    return picross::OutputGrid(0, 0, picross::Tile::UNKNOWN, "Invalid");
}

bool export_bitmap_pbm(const std::string& filepath, const picross::OutputGrid& grid, const picross::io::ErrorHandler& error_handler) noexcept
{
    try
    {
        pnm::pbm_image bitmap(grid.width(), grid.height());
        for (unsigned int y = 0u; y < grid.height(); y++)
            for (unsigned int x = 0u; x < grid.width(); x++)
            {
                bitmap[y][x] = (grid.get_tile(x, y) != picross::Tile::EMPTY);
            }
        pnm::write_pbm_binary(filepath, bitmap);
        return true;
    }
    catch (const std::exception& e)
    {
        error_handler(picross::io::ErrorCode::EXCEPTION, e.what());
    }
    return false;
}
