#pragma once

#include <imgui.h>

#include <picross/picross.h>

#include <mutex>
#include <string>
#include <thread>

class PicrossFile;

class GridWindow
{
public:
    GridWindow(PicrossFile& file, picross::InputGrid&& grid);
    ~GridWindow();
    GridWindow(const GridWindow&) = delete;
    GridWindow& operator=(const GridWindow&) = delete;

    void visit(bool& canBeErased);

private:
    void solve_picross_grid();

private:
    PicrossFile& file;
    picross::InputGrid grid;
    std::string title;
    std::thread solver_thread;
    std::mutex text_buffer_lock;
    ImGuiTextBuffer text_buffer;
};


