#pragma once

#include "err_window.h"
#include "grid_window.h"

#include <memory>
#include <string>
#include <string_view>

class Settings;

class BitmapFile
{
public:
    explicit BitmapFile(std::string_view path);
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
