#pragma once

#include <picross/picross.h>

#include <imgui.h>

#include <mutex>
#include <string>
#include <thread>

class PicrossFile;
class Settings;

class GridWindow
{
public:
    GridWindow(PicrossFile& file, picross::InputGrid&& grid);
    ~GridWindow();
    GridWindow(const GridWindow&) = delete;
    GridWindow& operator=(const GridWindow&) = delete;

    void visit(bool& canBeErased, Settings& settings);

private:
    void solve_picross_grid();

private:
    PicrossFile& file;
    picross::InputGrid grid;
    std::string title;
    std::thread solver_thread;
    std::mutex lock_mutex;
    ImGuiTextBuffer text_buffer;
    std::vector<picross::OutputGrid> solutions;
    std::vector<std::string> tabs;
};
