cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(picross_solver
        VERSION 0.1.0
)

option(PICROSS_BUILD_APP "Build the test application" OFF)

add_subdirectory(src/picross)

if(PICROSS_BUILD_APP)
    add_subdirectory(src/cli)
endif()

set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE)
include(CPack)
