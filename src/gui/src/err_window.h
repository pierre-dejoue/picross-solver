#pragma once

#include <imgui.h>

#include <mutex>
#include <string>
#include <string_view>

class ErrWindow
{
public:
    explicit ErrWindow(std::string_view filename);
    ErrWindow(const ErrWindow&) = delete;
    ErrWindow& operator=(const ErrWindow&) = delete;

    void visit(bool& canBeErased);

    void print(std::string_view msg);

private:
    std::string title;
    std::mutex text_buffer_lock;
    ImGuiTextBuffer text_buffer;
};
