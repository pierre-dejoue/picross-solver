// Copyright (c) 2023 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#pragma once

#include <stdutils/io.h>

#include <string_view>
#include <utility>

struct GLFWOptions
{
    std::string_view title{};
    bool maximize_window{false};
};

// Wrapper class for GLFW initialization and window
struct GLFWwindow;
class GLFWWindowContext
{
public:
    struct WindowStatus
    {
        bool is_minimized{false};
        bool is_maximized{false};
    };

    GLFWWindowContext(int width, int height, const GLFWOptions& options, const stdutils::io::ErrorHandler* err_handler = nullptr);
    ~GLFWWindowContext();
    GLFWwindow* window() { return m_window_ptr; }
    WindowStatus window_status() const;
    std::pair<int, int> framebuffer_size() const;
    std::pair<int, int> window_size() const;

    // Ratio between the framebuffer coordinates and the screen coordinates. Supposedly the same on the X and Y axis
    float get_framebuffer_scale() const;

private:
    GLFWwindow*     m_window_ptr;
    bool            m_glfw_init;
};

const char* glsl_version();
