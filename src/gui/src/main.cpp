/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Implementation of a GUI for the solver of Picross puzzles (nonograms)
 *
 * Copyright (c) 2021 Pierre DEJOUE
 ******************************************************************************/
#include <cassert>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <tuple>
#include <vector>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>
#include <portable-file-dialogs.h>

#include <picross/picross.h>
#include <picross/picross_io.h>


namespace
{

void glfw_error_callback(int error, const char* description)
{
    std::cerr << "Glfw Error " << error << ": " << description << std::endl;
}

struct PicrossFile
{
    explicit PicrossFile(const std::string& path)
        : filePath(path)
        , isFileOpen(false)
        , isWindowOpen(true)
    {}

    std::string filePath;
    bool isFileOpen;
    bool isWindowOpen;
    std::mutex textBufferLock;
    ImGuiTextBuffer textBuffer;
};

void picrossSolveGrids(PicrossFile* picrossFile)
{
    assert(picrossFile != nullptr);

    std::vector<picross::InputGrid> grids_to_solve = picross::parse_input_file(picrossFile->filePath, [picrossFile](const std::string& msg, picross::ExitCode)
    {
        std::lock_guard<std::mutex> lock(picrossFile->textBufferLock);
        picrossFile->textBuffer.append(msg.c_str());
    });

    const auto solver = picross::get_ref_solver();
    unsigned count_grids = 0u;

    for (const auto& grid_input : grids_to_solve)
    {
        {
            std::lock_guard<std::mutex> lock(picrossFile->textBufferLock);
            picrossFile->textBuffer.appendf("GRID %d: %s\n", ++count_grids, grid_input.name.c_str());
        }

        /* Sanity check of the input data */
        bool check;
        std::string check_msg;
        std::tie(check, check_msg) = picross::check_grid_input(grid_input);
        if (check)
        {
            std::vector<picross::OutputGrid> solutions = solver->solve(grid_input);
            if (solutions.empty())
            {
                std::lock_guard<std::mutex> lock(picrossFile->textBufferLock);
                picrossFile->textBuffer.appendf(" > Could not solve that grid :-(\n");
            }
            else
            {
                {
                    std::lock_guard<std::mutex> lock(picrossFile->textBufferLock);
                    picrossFile->textBuffer.appendf(" > Found %d solution(s) :\n", solutions.size());
                }
                for (const auto& solution : solutions)
                {
                    assert(solution.is_solved());
                    std::ostringstream oss;
                    solution.print(oss);
                    {
                        std::lock_guard<std::mutex> lock(picrossFile->textBufferLock);
                        picrossFile->textBuffer.append(oss.str().c_str());
                    }
                }
            }
        }
        else
        {
            std::lock_guard<std::mutex> lock(picrossFile->textBufferLock);
            picrossFile->textBuffer.appendf(" > Invalid grid. Error message: %s\n", check_msg.c_str());
        }
    }
}

void picrossFileWindow(PicrossFile& picrossFile)
{
    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(picrossFile.filePath.c_str(), &picrossFile.isWindowOpen))
    {
        ImGui::End();
        return;
    }

    if (!picrossFile.isFileOpen)
    {
        picrossFile.isFileOpen = true;
        std::thread th(picrossSolveGrids, &picrossFile);
        th.detach();
    }

    {
        std::lock_guard<std::mutex> lock(picrossFile.textBufferLock);
        ImGui::TextUnformatted(picrossFile.textBuffer.begin(), picrossFile.textBuffer.end());
    }
    ImGui::End();

}

} // Anonymous namespace


int main(int argc, char *argv[])
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Picross Solver", NULL, NULL);
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

    // Main loop
    std::vector<std::unique_ptr<PicrossFile>> openPicrossFiles;

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
                    const auto paths = pfd::open_file("Select a Picross file").result();
                    for (const auto path : paths)
                    {
                        std::cout << "User selected file " << path << "\n";
                        openPicrossFiles.emplace_back(std::make_unique<PicrossFile>(path));
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

        // Picross files windows
        for (auto it = std::begin(openPicrossFiles); it != std::end(openPicrossFiles);)
        {
            if ((*it)->isWindowOpen)
            {
                picrossFileWindow(**it);
                it++;
            }
            else
            {
                it = openPicrossFiles.erase(it);
            }
        }

        // Dear Imgui Demo
        ImGui::ShowDemoWindow();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        const ImVec4 clear_color = ImVec4(0.35f, 0.55f, 0.60f, 1.00f);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

