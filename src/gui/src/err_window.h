#pragma once

#include <imgui.h>

#include <mutex>
#include <string>

class PicrossFile;

class ErrWindow
{
public:
    ErrWindow(PicrossFile& file);
    ErrWindow(const ErrWindow&) = delete;
    ErrWindow& operator=(const ErrWindow&) = delete;

    void visit(bool& canBeErased);

    void print(const std::string& msg);

private:
    PicrossFile& file;
    std::string title;
    std::mutex text_buffer_lock;
    ImGuiTextBuffer text_buffer;
};


