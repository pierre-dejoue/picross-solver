#pragma once

#include "imgui_helpers.h"

#include <stdutils/io.h>

ImGuiImage make_app_logo(DearImGuiContext& imgui_context, const stdutils::io::ErrorHandler& err_handler);
