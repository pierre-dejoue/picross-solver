cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(picross_solver)

option(PICROSS_BUILD_APP "Build the test application" OFF)


#
# Build the library
#
set(LIB_SOURCES
    src/picross/src/picross_solver.cpp
)

file(GLOB LIB_HEADERS src/picross/include/picross/*.h src/picross/src/*.h)

add_library(picross STATIC ${LIB_SOURCES} ${LIB_HEADERS})
target_include_directories(picross
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/picross/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/picross/src
)


#
# Build the CLI application
#
if(PICROSS_BUILD_APP)
    set(CLI_SOURCES
        src/cli/src/main.cpp
    )

    file(GLOB CLI_HEADERS src/cli/src/*.h)

    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY bin/)
    add_executable(picross_solver ${CLI_SOURCES} ${CLI_HEADERS})
    target_link_libraries(picross_solver PRIVATE picross)
endif()
