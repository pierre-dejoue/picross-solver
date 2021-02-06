#
# Build the graphical application
#

# Dependencies
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
include(imgui)

set(GUI_SOURCES
    src/main.cpp
)

file(GLOB GUI_HEADERS src/*.h)

add_executable(picross_solver ${GUI_SOURCES} ${GUI_HEADERS})

target_link_libraries(picross_solver
    PRIVATE
    picross
    imgui
)

install(TARGETS picross_solver)
