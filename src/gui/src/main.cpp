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
// NB: No OpenGL loader here: This project mainly relies on the drawing features provided by Dear ImGui.
// Dear ImGui embeds its own minimal loader for the OpenGL 3.x functions it needs.
// See: https://github.com/ocornut/imgui/issues/4445 "OpenGL backend now embeds its own GL loader"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <vector>


namespace {

void err_callback(stdutils::io::SeverityCode sev, std::string_view msg)
{
    std::cerr << stdutils::io::str_severity_code(sev) << ": " << msg << std::endl;
}

namespace details {

    std::string s_project_title()
    {
        std::stringstream title;
        title << "Picross Solver " << picross::get_version_string();
        return title.str();
    }

} // namespace details

std::string_view project_title()
{
    static std::string project_title = details::s_project_title();
    return project_title;
}

// Application windows
struct AppWindows
{
    std::unique_ptr<SettingsWindow> settings;
    std::vector<std::unique_ptr<PicrossFile>> picross;
    struct
    {
        WindowLayout settings;
    } layout;
};

namespace shortcut {

const ImGui::KeyShortcut& open()
{
    constexpr ImGuiKeyChord key_chord = ImGuiMod_Ctrl | ImGuiKey_O;
#if defined(__APPLE__)
    static ImGui::KeyShortcut shortcut(key_chord, "Cmd+O");
#else
    static ImGui::KeyShortcut shortcut(key_chord, "Ctrl+O");
#endif
    return shortcut;
}

const ImGui::KeyShortcut& quit()
{
#if defined(__APPLE__)
    static ImGui::KeyShortcut shortcut(ImGuiMod_Ctrl | ImGuiKey_Q, "Cmd+Q");
#else
    static ImGui::KeyShortcut shortcut(ImGuiMod_Alt | ImGuiKey_F4, "Alt+F4");
#endif
    return shortcut;
}

} // namespace shortcut

namespace menu_bar {

void action_open_picross_file(AppWindows& windows)
{
    const auto paths = pfd::open_file("Select a Picross file", "",
        { "Picross file", "*.txt *.nin *.non", "All files", "*" }).result();
    for (const auto& path : paths)
    {
        const auto format = picross::io::picross_file_format_from_file_extension(path);
        std::cout << "User selected file " << path << " (format: " << format << ")" << std::endl;
        windows.picross.emplace_back(std::make_unique<PicrossFile>(path, format));
    }
}

void action_open_bitmap(AppWindows& windows)
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

void action_import_solution(AppWindows& windows)
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

void display(AppWindows& windows, bool& application_should_close, bool& gui_dark_mode)
{
    application_should_close = false;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open", shortcut::open().label))
            {
                action_open_picross_file(windows);
            }
            if (ImGui::MenuItem("Import bitmap"))
            {
                action_open_bitmap(windows);
            }
            if (ImGui::MenuItem("Import solution"))
            {
                action_import_solution(windows);
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
            if (ImGui::MenuItem("Quit", shortcut::quit().label))
            {
                application_should_close = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

} // namespace menu_bar

} // namespace

int main(int argc, char *argv[])
{
    UNUSED(argc); UNUSED(argv);
    stdutils::io::ErrorHandler err_handler(err_callback);

    // Setup main window
    GLFWWindowContext glfw_context = [&err_handler]() {
        constexpr int WINDOW_WIDTH = 1280;
        constexpr int WINDOW_HEIGHT = 720;
        GLFWOptions options;
        options.title = project_title();
        options.maximize_window = true;
        GLFWWindowContext context(WINDOW_WIDTH, WINDOW_HEIGHT, options, &err_handler);
        return context;
    }();
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
            menu_bar::display(windows, app_should_close, gui_dark_mode);
            if (app_should_close)
                glfwSetWindowShouldClose(glfw_context.window(), 1);
        }

        // Key Shorcuts
        if (ImGui::IsKeyChordPressed(shortcut::open().key_chord))
        {
            menu_bar::action_open_picross_file(windows);
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
            windows.settings->visit(can_be_erased, windows.layout.settings);
            assert(can_be_erased == false);     // Always ON
        }

#if PICROSS_GUI_IMGUI_DEMO_FLAG
        // Dear Imgui Demo
        ImGui::ShowDemoWindow();
#endif

        // We delay the Settings window opening until after the first ImGui rendering pass so that the actual work space
        // is known on first visit(). This is important for ImGui::Set* functions with flag ImGuiCond_Once.
        if (!windows.settings) { windows.settings = std::make_unique<SettingsWindow>(settings); }

        // Rendering
        const auto [display_w, display_h] = glfw_context.framebuffer_size();
        glViewport(0, 0, display_w, display_h);
        const auto clear_color = get_background_color(gui_dark_mode);
        glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
        glClear(GL_COLOR_BUFFER_BIT);
        dear_imgui_context.render();
        glfwSwapBuffers(glfw_context.window());
    } // while (!glfwWindowShouldClose(window))

    return EXIT_SUCCESS;
}
