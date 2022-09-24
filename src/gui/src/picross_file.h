#pragma once

#include "err_window.h"
#include "grid_window.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

class Settings;

class PicrossFile
{
public:
    explicit PicrossFile(std::string_view path);
    ~PicrossFile();

    void visit_windows(bool& canBeErased, Settings& settings);

private:
    ErrWindow& get_err_window();

private:
    std::string file_path;
    bool is_file_open;
    std::vector<std::unique_ptr<GridWindow>> windows;
    std::unique_ptr<ErrWindow> err_window;
};
