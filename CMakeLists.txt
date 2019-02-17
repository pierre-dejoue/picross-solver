cmake_minimum_required(VERSION 3.0)


set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


project(picross_solver)


set(SOURCES
    src/main.cpp
    src/picross_solver.cpp
)


set(HEADERS
    src/picross_solver.h
)


set (CMAKE_RUNTIME_OUTPUT_DIRECTORY bin/)


add_executable(picross_solver ${SOURCES} ${HEADERS})
target_include_directories(picross_solver PRIVATE src/)
