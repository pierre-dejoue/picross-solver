#include "err_window.h"

ErrWindow::ErrWindow(std::string_view filename)
    : title(std::string("Errors in file ") + filename.data())
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

void ErrWindow::print(std::string_view msg)
{
    std::lock_guard<std::mutex> lock(text_buffer_lock);
    text_buffer.appendf("%s\n", msg.data());
}
