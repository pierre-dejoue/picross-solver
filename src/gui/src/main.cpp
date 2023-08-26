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
#include "style.h"
#include "window_layout.h"

#include <picross/picross.h>
#include <stdutils/io.h>
#include <stdutils/macros.h>

// Order matters in this section
#include <imgui_wrap.h>
#include "imgui_helpers.h"
#include <pfd_wrap.h>
#include <GLFW/glfw3.h>
#include "glfw_context.h"
// NB: No OpenGL loader here: This project only relies on the drawing features provided by Dear ImGui.
// Dear ImGui embeds its own minimal loader for the OpenGL 3.x functions it needs.
// See: https://github.com/ocornut/imgui/issues/4445 "OpenGL backend now embeds its own GL loader"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <vector>


namespace
{

void err_callback(stdutils::io::SeverityCode sev, std::string_view msg)
{
    std::cerr << stdutils::io::str_severity_code(sev) << ": " << msg << std::endl;
}

std::string project_title()
{
    std::stringstream title;
    title << "Picross Solver " << picross::get_version_string();
    return title.str();
}

// Application windows
struct AppWindows
{
    std::unique_ptr<SettingsWindow> settings;
    std::vector<std::unique_ptr<PicrossFile>> picross;
};

void main_menu_bar(AppWindows& windows, bool& application_should_close, bool& gui_dark_mode)
{
    application_should_close = false;
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
                if (ImGui::MenuItem("Dark Mode", "", &gui_dark_mode))
                {
                    imgui_set_style(gui_dark_mode);
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                application_should_close = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

} // Anonymous namespace


int main(int argc, char *argv[])
{
    UNUSED(argc); UNUSED(argv);

    // Setup main window
    constexpr int WINDOW_WIDTH = 1280;
    constexpr int WINDOW_HEIGHT = 720;
    stdutils::io::ErrorHandler err_handler(err_callback);
    GLFWWindowContext glfw_context(WINDOW_WIDTH, WINDOW_HEIGHT, project_title().data(), &err_handler);
    if (glfw_context.window() == nullptr)
        return EXIT_FAILURE;

    // Setup Dear ImGui context
    bool any_fatal_err = false;
    const DearImGuiContext dear_imgui_context(glfw_context.window(), any_fatal_err);
    if (any_fatal_err)
        return EXIT_FAILURE;

    // Print out version information
    std::cout << project_title() << std::endl;
    dear_imgui_context.backend_info(std::cout);

    // Style
    bool gui_dark_mode = false;
    imgui_set_style(gui_dark_mode);

    // Application Settings
    Settings settings;

    // Application Windows
    AppWindows windows;

    // Main loop
    while (!glfwWindowShouldClose(glfw_context.window()))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

         // Start the Dear ImGui frame
        dear_imgui_context.new_frame();

        // Main menu
        {
            bool app_should_close = false;
            main_menu_bar(windows, app_should_close, gui_dark_mode);
            if (app_should_close)
                glfwSetWindowShouldClose(glfw_context.window(), 1);
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
        int display_w, display_h;
        glfwGetFramebufferSize(glfw_context.window(), &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        const auto clear_color = get_background_color(gui_dark_mode);
        glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
        glClear(GL_COLOR_BUFFER_BIT);
        dear_imgui_context.render();
        glfwSwapBuffers(glfw_context.window());
    } // while (!glfwWindowShouldClose(window))

    return EXIT_SUCCESS;
}

