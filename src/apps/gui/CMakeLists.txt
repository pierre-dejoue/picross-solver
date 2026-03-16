#
# Build the graphical application
#

# Dependencies
find_package(OpenGL REQUIRED)       # Necessary because we do not use a loader (such as gl3w or glad)
include(pfd)

set(GUI_SOURCES
    src/about_window.cpp
    src/bitmap_io_raw.cpp
    src/bitmap_to_glfw_image.cpp
    src/draw_grid.cpp
    src/embedded_file.cpp
    src/err_window.cpp
    src/glfw_context.cpp
    src/imgui_helpers.cpp
    src/goal_window.cpp
    src/grid_info.cpp
    src/grid_window.cpp
    src/logo.cpp
    src/logo_file.cpp
    src/main.cpp
    src/picross_file.cpp
    src/settings.cpp
    src/settings_window.cpp
    src/style.cpp
    src/window_icon.cpp
)

if(NOT APPLE)
    list(APPEND GUI_SOURCES
        src/window_icon_files.cpp
    )
endif()

file(GLOB GUI_HEADERS src/*.h)

if(MSVC)
    configure_file(src/icon.rc.in src/icon.rc @ONLY)
    set(APP_ICON ${CMAKE_CURRENT_BINARY_DIR}/src/icon.rc)
else()
    set(APP_ICON "")
endif()

add_executable(picross_solver
    ${GUI_SOURCES}
    ${GUI_HEADERS}
    ${APP_ICON}
)

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
