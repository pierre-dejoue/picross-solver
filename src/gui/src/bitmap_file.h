#pragma once

#include "err_window.h"
#include "grid_window.h"

#include <memory>
#include <string>

class Settings;

class BitmapFile
{
public:
    explicit BitmapFile(const std::string& path);
    ~BitmapFile();

    void visit_windows(bool& canBeErased, Settings& settings);

private:
    ErrWindow& get_err_window();

private:
    std::string file_path;
    bool is_file_open;
    std::unique_ptr<GridWindow> window;
    std::unique_ptr<ErrWindow> err_window;
};
