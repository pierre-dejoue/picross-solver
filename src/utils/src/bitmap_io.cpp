#include <utils/bitmap_io.h>

#include <utils/strings.h>

#include <pnm.hpp>

#include <cassert>
#include <cstddef>
#include <exception>

std::unique_ptr<picross::OutputGrid> import_bitmap_pbm(const std::string& filepath, const picross::ErrorHandler& error_handler) noexcept
{
    try
    {
        const pnm::pbm_image bitmap = pnm::read_pbm(filepath);
        const auto grid_name = file_name_wo_extension(filepath);
        auto output_grid = std::make_unique<picross::OutputGrid>(bitmap.width(), bitmap.height(), grid_name);
        assert(output_grid);
        std::size_t y = 0u;
        for (const auto& line : bitmap.lines())
        {
            std::size_t x = 0u;
            for (const auto& pix : line)
            {
                output_grid->set(x++, y, pix.value ? picross::Tile::ONE : picross::Tile::ZERO);
            }
            y++;
        }
        return output_grid;
    }
    catch(const std::exception& e)
    {
        error_handler(e.what(), 0);
    }
    return nullptr;
}

void export_bitmap_pbm(const std::string& filepath, const picross::OutputGrid& grid, const picross::ErrorHandler& error_handler) noexcept
{
    try
    {
        pnm::pbm_image bitmap(grid.get_width(), grid.get_height());
        for (unsigned int y = 0u; y < grid.get_height(); y++)
            for (unsigned int x = 0u; x < grid.get_width(); x++)
            {
                bitmap[x][y] = (grid.get(x, y) != picross::Tile::ZERO);
            }
        pnm::write_pbm_binary(filepath, bitmap);
    }
    catch (const std::exception& e)
    {
        error_handler(e.what(), 0);
    }
}