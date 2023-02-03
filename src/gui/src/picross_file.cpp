#include "picross_file.h"

#include <picross/picross.h>
#include <utils/strings.h>

#include <cassert>
#include <iostream>
#include <iterator>


PicrossFile::PicrossFile(std::string_view path, picross::io::PicrossFileFormat format)
    : file_path(path)
    , file_format(format)
    , is_file_open(false)
    , windows()
{}

PicrossFile::~PicrossFile() = default;

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
        std::optional<picross::OutputGrid> goal;
        std::vector<picross::InputGrid> grids_to_solve = picross::io::parse_picross_file(file_path, file_format, goal, err_handler);
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