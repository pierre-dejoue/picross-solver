#pragma once

#include "err_window.h"
#include "grid_window.h"

#include <memory>
#include <string>
#include <vector>

class Settings;

class PicrossFile
{
public:
    PicrossFile(const std::string& path);
    ~PicrossFile();

    const std::string& get_file_path() const;

    void visit_windows(bool& canBeErased, Settings& settings);

private:
    ErrWindow& get_err_window();

private:
    std::string file_path;
    bool is_file_open;
    std::vector<std::unique_ptr<GridWindow>> windows;
    std::unique_ptr<ErrWindow> err_window;
};
