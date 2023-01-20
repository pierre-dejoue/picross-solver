#pragma once

#include "err_window.h"
#include "grid_window.h"

#include <utils/picross_file_io.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

class Settings;

class PicrossFile
{
public:
    explicit PicrossFile(std::string_view path, picross::io::PicrossFileFormat format);
    ~PicrossFile();

    void visit_windows(bool& canBeErased, Settings& settings);

private:
    ErrWindow& get_err_window();

private:
    std::string file_path;
    picross::io::PicrossFileFormat file_format;
    bool is_file_open;
    std::vector<std::unique_ptr<GridWindow>> windows;
    std::unique_ptr<ErrWindow> err_window;
};
