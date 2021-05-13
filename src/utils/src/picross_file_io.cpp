#include <utils/picross_file_io.h>

#include <utils/strings.h>

#include <exception>
#include <fstream>

void save_picross_file(const std::string& filepath, const picross::InputGrid& grid, const picross::io::ErrorHandler& error_handler) noexcept
{
    const std::string ext = str_tolower(file_extension(filepath));

    try
    {
        std::ofstream out(filepath);
        if (out.good())
        {
            if (ext == "non")
                picross::io::write_input_grid_non_format(out, grid);
            else
                picross::io::write_input_grid(out, grid);
        }
        else
        {
            error_handler("Error writing file " + filepath, 0);
        }
    }
    catch (const std::exception& e)
    {
        error_handler(e.what(), 0);
    }
}
