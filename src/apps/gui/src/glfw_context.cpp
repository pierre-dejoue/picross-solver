// Copyright (c) 2023 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#include "glfw_context.h"

#include <GLFW/glfw3.h>

#include <cassert>

namespace {

// For GLFW on macOS: The supported versions are 3.2 on 10.7 Lion and 3.3 and 4.1 on 10.9 Mavericks.
constexpr int TARGET_OPENGL_MAJOR = 3;
constexpr int TARGET_OPENGL_MINOR = 2;
constexpr bool TARGET_OPENGL_CORE_PROFILE = true;                   // 3.2+ only
constexpr bool TARGET_OPENGL_FORWARD_COMPATIBILITY = true;          // 3.0 only, recommended for macOS
constexpr const char* TARGET_GLSL_VERSION_STR = "#version 150";

// GLFW error handling function
stdutils::io::ErrorHandler s_glfw_err_handler;
void glfw_error_callback(int error, const char* description)
{
    if(!s_glfw_err_handler) { return; }
    std::stringstream out;
    out << "GLFW Error " << error << ": " << description;
    s_glfw_err_handler(stdutils::io::Severity::ERR, out.str());
}

} // namespace

GLFWWindowContext::GLFWWindowContext(int width, int height, const GLFWOptions& options, const stdutils::io::ErrorHandler* err_handler)
    : m_window_ptr(nullptr)
    , m_glfw_init(false)
{
    static bool call_once = false;
    if (call_once)
        return;
    call_once = true;

    // Window title
    if (options.title.empty() && err_handler) { (*err_handler)(stdutils::io::Severity::WARN, "Window title is not specified"); }
    const std::string_view title = options.title.empty() ? "untitled" : options.title;

    if (err_handler)
    {
        s_glfw_err_handler = *err_handler;
        glfwSetErrorCallback(glfw_error_callback);
    }

    if (!glfwInit())
    {
        if (err_handler) { (*err_handler)(stdutils::io::Severity::FATAL, "GLFW failed to initialize"); }
        return;
    }
    m_glfw_init = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, TARGET_OPENGL_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, TARGET_OPENGL_MINOR);
    if constexpr (TARGET_OPENGL_CORE_PROFILE)
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if constexpr (TARGET_OPENGL_FORWARD_COMPATIBILITY)
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    if (options.maximize_window)
        glfwWindowHint(GLFW_MAXIMIZED, GL_TRUE);
    assert(title.data());
    m_window_ptr = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    if (m_window_ptr == nullptr)
    {
        if (err_handler) { (*err_handler)(stdutils::io::Severity::FATAL, "GLFW failed to create the window"); }
        return;
    }
    glfwMakeContextCurrent(m_window_ptr);
    glfwSwapInterval(1);
}

GLFWWindowContext::~GLFWWindowContext()
{
    if(m_window_ptr)
        glfwDestroyWindow(m_window_ptr);
    m_window_ptr = nullptr;
    if (m_glfw_init)
        glfwTerminate();
}

GLFWWindowContext::WindowStatus GLFWWindowContext::window_status() const
{
    WindowStatus result;
    assert(m_window_ptr);
    result.is_minimized = glfwGetWindowAttrib(m_window_ptr, GLFW_ICONIFIED);
    result.is_maximized = glfwGetWindowAttrib(m_window_ptr, GLFW_MAXIMIZED);
    return result;
}

std::pair<int, int> GLFWWindowContext::framebuffer_size() const
{
    std::pair<int, int> sz(0, 0);
    assert(m_window_ptr);
    glfwGetFramebufferSize(m_window_ptr, &sz.first, &sz.second);
    return sz;
}

std::pair<int, int> GLFWWindowContext::window_size() const
{
    std::pair<int, int> sz(0, 0);
    assert(m_window_ptr);
    glfwGetWindowSize(m_window_ptr, &sz.first, &sz.second);
    return sz;
}

float GLFWWindowContext::get_framebuffer_scale() const
{
    float scale{1.f};
    assert(m_window_ptr);

    std::pair<int, int> fb_sz(0, 0);
    glfwGetFramebufferSize(m_window_ptr, &fb_sz.first, &fb_sz.second);
    std::pair<int, int> window_sz(0, 0);
    glfwGetWindowSize(m_window_ptr, &window_sz.first, &window_sz.second);

    if (window_sz.first == 0 || window_sz.second == 0)
        return scale;

    const std::pair<float, float> fb_scale(
        static_cast<float>(fb_sz.first)  / static_cast<float>(window_sz.first),
        static_cast<float>(fb_sz.second) / static_cast<float>(window_sz.second)
    );

    // If the scales are different on the X and Y axis, log an error and arbitrarily use scale.x
    if (fb_scale.first != fb_scale.second && s_glfw_err_handler)
    {
        std::stringstream out;
        out << "Different coordinate scales on the X and Y axis";
        out << "; Framebuffer: " << fb_sz.first << "x" << fb_sz.second;
        out << "; Window: " << window_sz.first << "x" << window_sz.second;
        s_glfw_err_handler(stdutils::io::Severity::ERR, out.str());
    }
    scale = fb_scale.first;

    assert(scale > 0.f);
    return scale;
}

const char* glsl_version()
{
    return TARGET_GLSL_VERSION_STR;
}
