#include "picross_file.h"

#include <picross/picross_io.h>
#include <utils/strings.h>

#include <cassert>
#include <iostream>
#include <iterator>

PicrossFile::PicrossFile(std::string_view path)
    : file_path(path)
    , is_file_open(false)
    , windows()
{}

PicrossFile::~PicrossFile()
{
    windows.clear();
    std::cerr << "End file " << file_path << std::endl;
}

void PicrossFile::visit_windows(bool& canBeErased, Settings& settings)
{
    canBeErased = false;
    if (!is_file_open)
    {
        is_file_open = true;
        const auto err_handler = [this](std::string_view msg, picross::io::ExitCode)
        {
            this->get_err_window().print(msg);
        };
        std::vector<picross::InputGrid> grids_to_solve = str_tolower(file_extension(file_path)) == "non"
            ? picross::io::parse_input_file_non_format(file_path, err_handler)
            : picross::io::parse_input_file(file_path, err_handler);

        windows.reserve(grids_to_solve.size());
        for (auto& grid : grids_to_solve)
        {
            windows.push_back(std::make_unique<GridWindow>(std::move(grid), file_name(file_path)));
        }
    }
    else
    {
        canBeErased = true;

        // Grid windows
        for (auto it = std::begin(windows); it != std::end(windows);)
        {
            bool windowCanBeErased = false;
            (*it)->visit(windowCanBeErased, settings);
            canBeErased &= windowCanBeErased;
            it = windowCanBeErased ? windows.erase(it) : std::next(it);
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

ErrWindow& PicrossFile::get_err_window()
{
    if (!err_window)
    {
        err_window = std::make_unique<ErrWindow>(file_path);
    }
    assert(err_window);
    return *err_window.get();
}