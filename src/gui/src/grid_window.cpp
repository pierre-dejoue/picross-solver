#include "grid_window.h"

#include "err_window.h"
#include "picross_file.h"

#include <sstream>

GridWindow::GridWindow(PicrossFile& file, picross::InputGrid&& grid)
    : file(file)
    , grid(std::move(grid))
    , title()
    , solver_thread()
    , text_buffer_lock()
    , text_buffer()
{
    title = this->grid.name + " (" + file.get_file_path() + ")";
    std::swap(solver_thread, std::thread(&GridWindow::solve_picross_grid, this));
}

GridWindow::~GridWindow()
{
    solver_thread.join();
}

void GridWindow::visit(bool& canBeErased)
{
    ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_FirstUseEver);

    bool isWindowOpen = true;
    if (!ImGui::Begin(title.c_str(), &isWindowOpen) || !isWindowOpen)
    {
        canBeErased = true;
        ImGui::End();
        return;
    }
    canBeErased = false;

    {
        std::lock_guard<std::mutex> lock(text_buffer_lock);
        ImGui::TextUnformatted(text_buffer.begin(), text_buffer.end());
    }
    ImGui::End();
}

void GridWindow::solve_picross_grid()
{
    const auto solver = picross::get_ref_solver();
    unsigned count_grids = 0u;

    /* Sanity check of the input data */
    bool check;
    std::string check_msg;
    std::tie(check, check_msg) = picross::check_grid_input(grid);
    if (check)
    {
        std::vector<picross::OutputGrid> solutions = solver->solve(grid);
        if (solutions.empty())
        {
            std::lock_guard<std::mutex> lock(text_buffer_lock);
            text_buffer.appendf(" > Could not solve that grid :-(\n");
        }
        else
        {
            {
                std::lock_guard<std::mutex> lock(text_buffer_lock);
                text_buffer.appendf(" > Found %d solution(s) :\n", solutions.size());
            }
            for (const auto& solution : solutions)
            {
                assert(solution.is_solved());
                std::ostringstream oss;
                oss << solution << std::endl;
                {
                    std::lock_guard<std::mutex> lock(text_buffer_lock);
                    text_buffer.append(oss.str().c_str());
                }
            }
        }
    }
    else
    {
        std::lock_guard<std::mutex> lock(text_buffer_lock);
        text_buffer.appendf(" > Invalid grid. Error message: %s\n", check_msg.c_str());
    }
}