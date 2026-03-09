// Copyright (c) 2026 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#pragma once

struct ImGuiImage;

struct AboutWindow
{
    struct Input
    {
        const ImGuiImage* app_logo{nullptr};
    };

    // Return true if the window is to remain open
    static bool visit(const Input& input);
};
