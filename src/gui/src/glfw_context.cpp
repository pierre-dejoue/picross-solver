#include "glfw_context.h"


#include <GLFW/glfw3.h>


namespace {

constexpr int TARGET_OPENGL_MAJOR = 3;
constexpr int TARGET_OPENGL_MINOR = 0;
constexpr bool TARGET_OPENGL_CORE_PROFILE = false;                  // 3.2+ only
constexpr bool TARGET_OPENGL_FORWARD_COMPATIBILITY = true;          // 3.0 only, recommended for MacOS
constexpr const char* TARGET_GLSL_VERSION_STR = "#version 130";

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

GLFWWindowContext::GLFWWindowContext(int width, int height, const std::string_view& title, const stdutils::io::ErrorHandler* err_handler)
    : m_window_ptr(nullptr)
    , m_glfw_init(false)
{
    static bool call_once = false;
    if (call_once)
        return;
    call_once = true;

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

const char* glsl_version()
{
    return TARGET_GLSL_VERSION_STR;
}
