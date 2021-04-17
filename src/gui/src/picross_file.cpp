#include "picross_file.h"

#include <picross/picross_io.h>

#include <iostream>
#include <iterator>

PicrossFile::PicrossFile(const std::string& path)
    : file_path(path)
    , is_file_open(false)
    , windows()
{}

PicrossFile::~PicrossFile()
{
    std::cerr << "End file " << file_path << std::endl;
}

const std::string& PicrossFile::get_file_path() const
{
    return file_path;
}

void PicrossFile::visit_windows(bool& canBeErased, Settings& settings)
{
    canBeErased = false;
    if (!is_file_open)
    {
        is_file_open = true;
        std::vector<picross::InputGrid> grids_to_solve = picross::parse_input_file(file_path, [this](const std::string& msg, picross::ExitCode)
        {
            this->get_err_window().print(msg);
        });

        windows.reserve(grids_to_solve.size());
        for (auto& grid : grids_to_solve)
        {
            windows.push_back(std::make_unique<GridWindow>(std::move(grid), get_file_path()));
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
        err_window = std::make_unique<ErrWindow>(*this);
    }
    assert(err_window);
    return *err_window.get();
}