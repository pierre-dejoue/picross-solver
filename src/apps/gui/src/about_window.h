// Copyright (c) 2026 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#pragma once

struct ImGuiImage;
class DearImGuiContext;

struct AboutWindow
{
    struct Input
    {
        const ImGuiImage* app_logo{nullptr};
        DearImGuiContext* dear_imgui_context{nullptr};
    };

    // Return true if the window is to remain open
    static bool visit(const Input& input);
};
