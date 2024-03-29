cmake_minimum_required(VERSION 3.20)

project(picross_solver
    VERSION 0.3.1
)

option(PICROSS_BUILD_APP "Build the test application" OFF)
option(PICROSS_BUILD_CLI "Build the command line app" OFF)
option(PICROSS_BUILD_TESTS "Build and run unit tests" OFF)
option(PICROSS_BUILD_EXAMPLES "Build the examples" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/third_party/")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/config/")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Stdutils
add_subdirectory(src/stdutils)

# Picross library
add_subdirectory(src/picross)

# Utility library
if(PICROSS_BUILD_APP OR PICROSS_BUILD_CLI OR PICROSS_BUILD_TESTS)
    add_subdirectory(src/utils)
endif()

# Picross graphical app
if(PICROSS_BUILD_APP)
    add_subdirectory(src/gui/imgui)
    add_subdirectory(src/gui)
endif()

# Picross CLI
if(PICROSS_BUILD_CLI)
    add_subdirectory(src/cli)
endif()

# Tests
if(PICROSS_BUILD_TESTS)
    add_subdirectory(src/tests/stdutils)
    add_subdirectory(src/tests/picross)
endif()

# Examples
if(PICROSS_BUILD_EXAMPLES)
    add_subdirectory(src/examples)
endif()

install(EXPORT picross_project NAMESPACE picross:: DESTINATION lib/picross)
install(FILES ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION .)

set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE)
include(CPack)
