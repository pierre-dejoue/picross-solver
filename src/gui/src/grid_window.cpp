#include "grid_window.h"

#include "err_window.h"
#include "picross_file.h"

#include <sstream>
#include <utility>

GridWindow::GridWindow(PicrossFile& file, picross::InputGrid&& grid)
    : file(file)
    , grid(std::move(grid))
    , title()
    , solver_thread()
    , lock_mutex()
    , text_buffer()
    , solutions()
    , tabs()
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
    if (!ImGui::Begin(title.c_str(), &isWindowOpen))
    {
        // Collapsed
        canBeErased = !isWindowOpen;
        ImGui::End();
        return;
    }
    canBeErased = !isWindowOpen;

    // Text
    {
        std::lock_guard<std::mutex> lock(lock_mutex);
        ImGui::TextUnformatted(text_buffer.begin(), text_buffer.end());
    }

    // Solutions tabs
    {
        std::lock_guard<std::mutex> lock(lock_mutex);
        if (solutions.empty())
        {
            // No solutions, or not yet computed
            ImGui::End();
            return;
        }
    }

    // Solutions vector was swapped once, therefore can now being accessed without lock
    if (tabs.empty())
    {
        for (unsigned int idx = 1u; idx <= solutions.size(); ++idx)
        {
            tabs.emplace_back("Solution " + std::to_string(idx));
        }
    }
    if (ImGui::BeginTabBar("##TabBar"))
    {
        for (unsigned int idx = 0u; idx < solutions.size(); ++idx)
        {
            if (ImGui::BeginTabItem(tabs.at(idx).c_str()))
            {
                const auto& solution = solutions.at(idx);
                assert(solution.is_solved());
                std::ostringstream oss;
                oss << solution;
                ImGui::TextUnformatted(oss.str().c_str());
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void GridWindow::solve_picross_grid()
{
    const auto solver = picross::get_ref_solver();
    unsigned count_grids = 0u;

    // Sanity check of the input data
    bool check;
    std::string check_msg;
    std::tie(check, check_msg) = picross::check_grid_input(grid);
    if (check)
    {
        std::vector<picross::OutputGrid> local_solutions = solver->solve(grid);
        std::lock_guard<std::mutex> lock(lock_mutex);
        if (local_solutions.empty())
        {
            text_buffer.appendf("Could not solve that grid :-(\n");
        }
        else
        {
            // No text
            std::swap(solutions, local_solutions);
        }
    }
    else
    {
        std::lock_guard<std::mutex> lock(lock_mutex);
        text_buffer.appendf("Invalid grid. Error message: %s\n", check_msg.c_str());
    }
}