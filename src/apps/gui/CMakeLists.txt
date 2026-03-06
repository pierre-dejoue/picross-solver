#
# Build the graphical application
#

option(PICROSS_GUI_IMGUI_DEMO "Show ImGUI demo window" OFF)

# Dependencies
find_package(OpenGL REQUIRED)       # Necessary because we do not use a loader (such as gl3w or glad)
include(pfd)

set(GUI_SOURCES
    src/draw_grid.cpp
    src/err_window.cpp
    src/glfw_context.cpp
    src/imgui_helpers.cpp
    src/goal_window.cpp
    src/grid_info.cpp
    src/grid_window.cpp
    src/main.cpp
    src/picross_file.cpp
    src/settings.cpp
    src/settings_window.cpp
    src/style.cpp
)

file(GLOB GUI_HEADERS src/*.h)

add_executable(picross_solver ${GUI_SOURCES} ${GUI_HEADERS})

if(PICROSS_GUI_IMGUI_DEMO)
    target_compile_definitions(picross_solver
        PUBLIC
        PICROSS_GUI_IMGUI_DEMO_FLAG
    )
endif()

if(APPLE)
    # OpenGL API is deprecated since macOS 10.14
    target_compile_definitions(picross_solver
        PUBLIC
        GL_SILENCE_DEPRECATION
    )
endif()

target_include_directories(picross_solver
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_warnings(picross_solver ON)

target_link_libraries(picross_solver
    PRIVATE
    stdutils
    picross::picross
    picross::utils
    glfw
    imgui
    OpenGL::GL
    pfd
)

install(TARGETS picross_solver)
