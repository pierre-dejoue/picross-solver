#
# Build the graphical application
#

option(PICROSS_GUI_IMGUI_DEMO "Show ImGUI demo window" OFF)

# Dependencies
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
include(imgui)
include(pfd)

set(GUI_SOURCES
    src/err_window.cpp
    src/grid_window.cpp
    src/main.cpp
    src/picross_file.cpp
    src/settings.cpp
    src/settings_window.cpp
)

file(GLOB GUI_HEADERS src/*.h)

add_executable(picross_solver ${GUI_SOURCES} ${GUI_HEADERS})

if (PICROSS_GUI_IMGUI_DEMO)
    add_compile_definitions(PICROSS_GUI_IMGUI_DEMO)
endif()

target_link_libraries(picross_solver
    PRIVATE
    picross
    utils
    imgui
    portable_file_dialogs
)

install(TARGETS picross_solver)
