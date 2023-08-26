#include "imgui_helpers.h"

#include <imgui_wrap.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include "glfw_context.h"


namespace ImGui
{

void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

// Place the window in the working area, that is the position of the viewport minus task bars, menus bars, status bars, etc.
void SetNextWindowPosAndSize(const WindowLayout& window_layout, ImGuiCond cond)
{
    const auto work_tl_corner = ImGui::GetMainViewport()->WorkPos;
    const auto work_size = ImGui::GetMainViewport()->WorkSize;
    const ImVec2 tl_corner(window_layout.m_position.x + work_tl_corner.x, window_layout.m_position.y + work_tl_corner.y);
    ImGui::SetNextWindowPos(tl_corner, cond);
    ImGui::SetNextWindowSize(to_imgui_vec2(window_layout.window_size(to_screen_size(work_size))), cond);
}

} // namespace ImGui

DearImGuiContext::DearImGuiContext(GLFWwindow* glfw_window, bool& any_fatal_error) noexcept
{
    any_fatal_error = false;
    try
    {
        const bool versions_ok = IMGUI_CHECKVERSION();
        const auto* ctx = ImGui::CreateContext();

        // Setup Platform/Renderer backends
        const bool init_glfw = ImGui_ImplGlfw_InitForOpenGL(glfw_window, true);
        const bool init_opengl3 = ImGui_ImplOpenGL3_Init(glsl_version());

        any_fatal_error = !versions_ok || (ctx == nullptr) || !init_glfw || !init_opengl3;
    }
    catch(const std::exception&)
    {
        any_fatal_error = true;
    }
}

DearImGuiContext::~DearImGuiContext()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void DearImGuiContext::new_frame() const
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void DearImGuiContext::render() const
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DearImGuiContext::backend_info(std::ostream& out) const
{
    const ImGuiIO& io = ImGui::GetIO();
    out << "Dear ImGui " << IMGUI_VERSION
        << " (Backend platform: " << (io.BackendPlatformName ? io.BackendPlatformName : "NULL")
        << ", renderer: " << (io.BackendRendererName ? io.BackendRendererName : "NULL") << ")" << std::endl;
    out << "GLFW " << GLFW_VERSION_MAJOR << "." << GLFW_VERSION_MINOR << "." << GLFW_VERSION_REVISION << std::endl;
    out << "OpenGL Version " << glGetString(GL_VERSION) << std::endl;
    out << "OpenGL Renderer " << glGetString(GL_RENDERER) << std::endl;
}
