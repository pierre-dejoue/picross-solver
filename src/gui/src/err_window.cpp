#include "err_window.h"

#include "picross_file.h"

ErrWindow::ErrWindow(PicrossFile& file)
    : file(file)
    , title()
    , text_buffer_lock()
    , text_buffer()
{
    title = "Errors in file " + file.get_file_path();
}

void ErrWindow::visit(bool& canBeErased)
{
    ImGui::SetNextWindowSize(ImVec2(640, 300), ImGuiCond_FirstUseEver);

    bool isWindowOpen;
    if (!ImGui::Begin(title.c_str(), &isWindowOpen))
    {
        // Collapsed
        canBeErased = !isWindowOpen;
        ImGui::End();
        return;
    }
    canBeErased = !isWindowOpen;

    {
        std::lock_guard<std::mutex> lock(text_buffer_lock);
        ImGui::TextUnformatted(text_buffer.begin(), text_buffer.end());
    }
    ImGui::End();
}

void ErrWindow::print(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(text_buffer_lock);
    text_buffer.append(msg.c_str());
}
