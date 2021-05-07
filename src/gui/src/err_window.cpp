#include "err_window.h"

ErrWindow::ErrWindow(const std::string& filename)
    : title("Errors in file " + title)
    , text_buffer_lock()
    , text_buffer()
{
}

void ErrWindow::visit(bool& canBeErased)
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 300), ImVec2(FLT_MAX, 600));

    bool isWindowOpen;
    if (!ImGui::Begin(title.c_str(), &isWindowOpen, ImGuiWindowFlags_AlwaysAutoResize))
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
    text_buffer.appendf("%s\n", msg.c_str());
}
