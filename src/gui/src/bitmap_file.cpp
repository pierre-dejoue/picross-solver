#include "bitmap_file.h"

#include <picross/picross.h>
#include <utils/bitmap_io.h>
#include <utils/input_from_output.h>
#include <utils/strings.h>

#include <cassert>
#include <iostream>
#include <iterator>

BitmapFile::BitmapFile(std::string_view path)
    : file_path(path)
    , is_file_open(false)
    , window()
{}

BitmapFile::~BitmapFile()
{
    window.reset();
    std::cerr << "Close window for file " << file_path << std::endl;
}

void BitmapFile::visit_windows(bool& canBeErased, Settings& settings)
{
    canBeErased = false;
    if (!is_file_open)
    {
        is_file_open = true;
        const auto err_handler = [this](std::string_view msg, picross::io::ExitCode)
        {
            this->get_err_window().print(msg);
        };
        auto output_grid = import_bitmap_pbm(file_path, err_handler);
        if (output_grid)
        {
            auto grid = picross::build_input_grid_from(*output_grid);
            window = std::make_unique<GridWindow>(std::move(grid), file_name(file_path));
        }
    }
    else
    {
        canBeErased = true;

        // Grid windows
        if (window)
        {
            bool windowCanBeErased = false;
            window->visit(windowCanBeErased, settings);
            if (windowCanBeErased)
            {
                window.reset();
            }
            canBeErased &= windowCanBeErased;
        }

        // Error window
        if (err_window)
        {
            bool windowCanBeErased = false;
            err_window->visit(windowCanBeErased);
            if (windowCanBeErased)
            {
                err_window.reset();
            }
            canBeErased &= windowCanBeErased;
        }
    }
}

ErrWindow& BitmapFile::get_err_window()
{
    if (!err_window)
    {
        err_window = std::make_unique<ErrWindow>(file_path);
    }
    assert(err_window);
    return *err_window.get();
}