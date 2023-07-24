/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Implementation of a GUI for the solver of Picross puzzles (nonograms)
 *
 * Copyright (c) 2021 Pierre DEJOUE
 ******************************************************************************/

#include "picross_file.h"
#include "settings.h"

#include <picross/picross.h>

#include <portable-file-dialogs.h>      // Include before glfw3.h
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>

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

} // Anonymous namespace


int main(int argc, char *argv[])
{
    // Versions
    std::stringstream picross_title;
    picross_title << "Picross Solver " << picross::get_version_string();
    std::cout << picross_title.str() << std::endl;
    std::cout << "Dear ImGui " << IMGUI_VERSION << std::endl;

    // Setup main window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, picross_title.str().c_str(), NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;   // Unused

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    // Style
    bool imgui_dark_mode = false;
    imgui_set_style(imgui_dark_mode);

    // Open settings window
    Settings settings;
    settings.open_window();

    // Main loop
    std::vector<std::unique_ptr<PicrossFile>> picross_files;
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
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
                    for (const auto path : paths)
                    {
                        const auto format = picross::io::picross_file_format_from_filepath(path);
                        std::cout << "User selected file " << path << " (format: " << format << ")" << std::endl;
                        picross_files.emplace_back(std::make_unique<PicrossFile>(path, format));
                    }
                }
                if (ImGui::MenuItem("Import bitmap", "Ctrl+I"))
                {
                    const auto paths = pfd::open_file("Select a bitmap file", "",
                        { "PBM file", "*.pbm", "All files", "*" }).result();
                    for (const auto path : paths)
                    {
                        const auto format = picross::io::PicrossFileFormat::PBM;
                        std::cout << "User selected bitmap " << path << " (format: " << format << ")" << std::endl;
                        picross_files.emplace_back(std::make_unique<PicrossFile>(path, format));
                    }
                }
                if (ImGui::MenuItem("Import solution"))
                {
                    const auto paths = pfd::open_file("Select a solution file", "",
                        { "All files", "*" }).result();
                    for (const auto path : paths)
                    {
                        const auto format = picross::io::PicrossFileFormat::OutputGrid;
                        std::cout << "User selected solution file " << path << std::endl;
                        picross_files.emplace_back(std::make_unique<PicrossFile>(path, format));
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
        for (auto it = std::begin(picross_files); it != std::end(picross_files);)
        {
            bool can_be_erased = false;
            (*it)->visit_windows(can_be_erased, settings);
            it = can_be_erased ? picross_files.erase(it) : std::next(it);
        }

        // Settings window (always ON)
        {
            bool can_be_erased = false;
            settings.visit_window(can_be_erased);
        }

#if PICROSS_GUI_IMGUI_DEMO
        // Dear Imgui Demo
        ImGui::ShowDemoWindow();
#endif

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        const auto clear_color = static_cast<ImVec4>(imgui_dark_mode ? WindowMainBackgroundColor_Dark : WindowMainBackgroundColor_Classic);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);
    } // while (!glfwWindowShouldClose(window))

    // Cleanup
    picross_files.clear();
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

