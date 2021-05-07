#pragma once

#include <imgui.h>

#include <mutex>
#include <string>

class ErrWindow
{
public:
    explicit ErrWindow(const std::string& filename);
    ErrWindow(const ErrWindow&) = delete;
    ErrWindow& operator=(const ErrWindow&) = delete;

    void visit(bool& canBeErased);

    void print(const std::string& msg);

private:
    std::string title;
    std::mutex text_buffer_lock;
    ImGuiTextBuffer text_buffer;
};
