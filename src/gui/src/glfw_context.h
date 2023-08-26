#pragma once

#include <stdutils/io.h>
#include <string_view>

// Wrapper class for GLFW initialization and window
struct GLFWwindow;
class GLFWWindowContext
{
public:
    GLFWWindowContext(int width, int height, const std::string_view& title, const stdutils::io::ErrorHandler* err_handler = nullptr);
    ~GLFWWindowContext();
    GLFWwindow* window() { return m_window_ptr; }

private:
    GLFWwindow*     m_window_ptr;
    bool            m_glfw_init;
};

const char* glsl_version();
