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

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>
#include <portable-file-dialogs.h>

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

    //ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    // Style
    ImGui::StyleColorsClassic();
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImColor(29, 75, 99, 255);

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
                        { "Picross file", "*.txt *.non", "All files", "*" }).result();
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
                        std::cout << "User selected bitmap " << path << std::endl;
                        picross_files.emplace_back(std::make_unique<PicrossFile>(path, picross::io::PicrossFileFormat::PBM));
                    }
                }
                if (ImGui::MenuItem("Import solution"))
                {
                    const auto paths = pfd::open_file("Select a solution file", "",
                        { "All files", "*" }).result();
                    for (const auto path : paths)
                    {
                        std::cout << "User selected solution file " << path << std::endl;
                        picross_files.emplace_back(std::make_unique<PicrossFile>(path, picross::io::PicrossFileFormat::OutputGrid));
                    }
                }
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
            bool canBeErased = false;
            (*it)->visit_windows(canBeErased, settings);
            it = canBeErased ? picross_files.erase(it) : std::next(it);
        }

        // Settings window
        {
            bool canBeErased = false;
            settings.visit_window(canBeErased);
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
        const auto clear_color = static_cast<ImVec4>(ImColor(35, 92, 121, 255));
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

