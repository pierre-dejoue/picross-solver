/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Implementation of a GUI for the solver of Picross puzzles (nonograms)
 *
 * Copyright (c) 2021 Pierre DEJOUE
 ******************************************************************************/

#include "picross_file.h"
#include "settings.h"
#include "settings_window.h"

#include <picross/picross.h>
#include <stdutils/macros.h>

// Order matters in this section
#include <imgui_wrap.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <pfd_wrap.h>
#include <GLFW/glfw3.h>
// NB: No OpenGL loader here: this project only relies on the drawing features provided by Dear ImGui.
// Dear ImGui embeds its own minimal loader for the OpenGL 3.x functions it needs.
// See: https://github.com/ocornut/imgui/issues/4445 "OpenGL backend now embeds its own GL loader"

#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <vector>


namespace
{

void glfw_error_callback(int error, const char* description)
{
    std::cerr << "Glfw Error " << error << ": " << description << std::endl;
}

const ImColor WindowBackgroundColor_Classic(29, 75, 99, 255);
const ImColor WindowBackgroundColor_Dark(4, 8, 25, 255);
const ImColor WindowMainBackgroundColor_Classic(35, 92, 121, 255);
const ImColor WindowMainBackgroundColor_Dark(10, 25, 50, 255);

void imgui_set_style(bool dark_mode)
{
    if (dark_mode)
    {
        ImGui::StyleColorsDark();
        ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = WindowBackgroundColor_Dark;
    }
    else
    {
        ImGui::StyleColorsClassic();
        ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = WindowBackgroundColor_Classic;
    }
}

// Application windows
struct AppWindows
{
    std::unique_ptr<SettingsWindow> settings;
    std::vector<std::unique_ptr<PicrossFile>> picross;
};

} // Anonymous namespace


int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    // Versions
    std::stringstream picross_title;
    picross_title << "Picross Solver " << picross::get_version_string();
    std::cout << picross_title.str() << std::endl;

    // Setup main window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(1280, 720, picross_title.str().c_str(), nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Print out version information
    std::cout << "Dear ImGui " << IMGUI_VERSION << std::endl;
    std::cout << "GLFW " << GLFW_VERSION_MAJOR << "." << GLFW_VERSION_MINOR << "." << GLFW_VERSION_REVISION << std::endl;
    std::cout << "OpenGL Version " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Renderer " << glGetString(GL_RENDERER) << std::endl;

    // Style
    bool imgui_dark_mode = false;
    imgui_set_style(imgui_dark_mode);

    // Application Settings
    Settings settings;

    // Application Windows
    AppWindows windows;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Main menu
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open", "Ctrl+O"))
                {
                    const auto paths = pfd::open_file("Select a Picross file", "",
                        { "Picross file", "*.txt *.nin *.non", "All files", "*" }).result();
                    for (const auto& path : paths)
                    {
                        const auto format = picross::io::picross_file_format_from_filepath(path);
                        std::cout << "User selected file " << path << " (format: " << format << ")" << std::endl;
                        windows.picross.emplace_back(std::make_unique<PicrossFile>(path, format));
                    }
                }
                if (ImGui::MenuItem("Import bitmap", "Ctrl+I"))
                {
                    const auto paths = pfd::open_file("Select a bitmap file", "",
                        { "PBM file", "*.pbm", "All files", "*" }).result();
                    for (const auto& path : paths)
                    {
                        const auto format = picross::io::PicrossFileFormat::PBM;
                        std::cout << "User selected bitmap " << path << " (format: " << format << ")" << std::endl;
                        windows.picross.emplace_back(std::make_unique<PicrossFile>(path, format));
                    }
                }
                if (ImGui::MenuItem("Import solution"))
                {
                    const auto paths = pfd::open_file("Select a solution file", "",
                        { "All files", "*" }).result();
                    for (const auto& path : paths)
                    {
                        const auto format = picross::io::PicrossFileFormat::OutputGrid;
                        std::cout << "User selected solution file " << path << std::endl;
                        windows.picross.emplace_back(std::make_unique<PicrossFile>(path, format));
                    }
                }
                ImGui::Separator();
                if (ImGui::BeginMenu("Options"))
                {
                    if (ImGui::MenuItem("Dark Mode", "", &imgui_dark_mode))
                    {
                        imgui_set_style(imgui_dark_mode);
                    }
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Quit", "Alt+F4"))
                {
                    glfwSetWindowShouldClose(window, 1);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Picross files windows (one window per grid, so possibly multiple windows per file)
        for (auto it = std::begin(windows.picross); it != std::end(windows.picross);)
        {
            bool can_be_erased = false;
            (*it)->visit_windows(can_be_erased, settings);
            it = can_be_erased ? windows.picross.erase(it) : std::next(it);
        }

        // Settings window
        if (windows.settings)
        {
            bool can_be_erased = false;
            windows.settings->visit(can_be_erased);
            assert(can_be_erased == false);     // Always ON
        }

#if PICROSS_GUI_IMGUI_DEMO_FLAG
        // Dear Imgui Demo
        ImGui::ShowDemoWindow();
#endif

        // We delay the Settings window opening until after the first ImGui rendering pass so that the actual work space
        // is known on first visit. This may be important for ImGui::Set* functions with ImGuiCond_Once.
        if (!windows.settings) { windows.settings = std::make_unique<SettingsWindow>(settings); }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        const auto clear_color = static_cast<ImVec4>(imgui_dark_mode ? WindowMainBackgroundColor_Dark : WindowMainBackgroundColor_Classic);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    } // while (!glfwWindowShouldClose(window))

    // Cleanup
    windows.picross.clear();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

