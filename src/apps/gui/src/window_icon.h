#pragma once

#include <stdutils/io.h>

class GLFWWindowContext;
void set_window_icon(GLFWWindowContext& glfw_window_context, const stdutils::io::ErrorHandler& err_handler);
