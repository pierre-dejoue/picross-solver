#include "err_window.h"

#include <imgui.h>

#include <mutex>
#include <string>


struct ErrWindow::Impl
{
    std::string title;
    std::mutex text_buffer_lock;
    ImGuiTextBuffer text_buffer;
};

ErrWindow::ErrWindow(std::string_view filename)
    : pImpl(std::make_unique<Impl>())
{
    pImpl->title = std::string("Errors in file ") + filename.data();
}

ErrWindow::~ErrWindow() = default;

void ErrWindow::visit(bool& canBeErased)
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 300), ImVec2(FLT_MAX, 600));

    bool isWindowOpen;
    if (!ImGui::Begin(pImpl->title.c_str(), &isWindowOpen, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // Collapsed
        canBeErased = !isWindowOpen;
        ImGui::End();
        return;
    }
    canBeErased = !isWindowOpen;

    {
        std::lock_guard<std::mutex> lock(pImpl->text_buffer_lock);
        ImGui::TextUnformatted(pImpl->text_buffer.begin(), pImpl->text_buffer.end());
    }
    ImGui::End();
}

void ErrWindow::print(std::string_view msg)
{
    std::lock_guard<std::mutex> lock(pImpl->text_buffer_lock);
    pImpl->text_buffer.appendf("%s\n", msg.data());
}
