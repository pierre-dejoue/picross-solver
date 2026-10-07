cmake_minimum_required(VERSION 3.20)

# - On XCODE 14, 15 and 16 the range of supported deployment targets begins with macOS 10.13 High Sierra
#   Reference: https://developer.apple.com/support/xcode/
# - Support for std::filesystem::path however is only available starting from macOS 10.15 Catalina
# - With the arm64 architecture, the deployment target will be automagically set to 11.0, the first macOS version to support the M1 processor
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment target" FORCE)

project(picross_solver
    VERSION 0.3.1
)

option(PICROSS_BUILD_GUI "Build the GUI application" OFF)
option(PICROSS_BUILD_CLI "Build the command line app" OFF)
option(PICROSS_BUILD_TESTS "Build and run unit tests" OFF)
option(PICROSS_BUILD_EXAMPLES "Build the library usage examples" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THIS_PROJECT_ID "picross_solver")
set(THIS_PROJECT_NAME "Picross Solver")
set(THIS_PROJECT_OWNER "Pierre DEJOUE")
set(THIS_PROJECT_YEAR_RANGE "2010-2026")
set(THIS_PROJECT_SHORT_LICENSE "MIT license")
set(THIS_PROJECT_SHORT_COPYRIGHT "Copyright (c) ${THIS_PROJECT_YEAR_RANGE} ${THIS_PROJECT_VENDOR}")
set(THIS_PROJECT_COPYRIGHT "${THIS_PROJECT_SHORT_COPYRIGHT}. ${THIS_PROJECT_SHORT_LICENSE}.")
set(THIS_PROJECT_DOMAIN_ID "asteridesoftware")
set(THIS_PROJECT_DOMAIN_TLD "com")

# The target platform (e.g. "macos-arm64")
string(TOLOWER ${CMAKE_SYSTEM_NAME} THIS_PROJECT_TARGET_PLATFORM)
if(MSVC)
    set(THIS_PROJECT_TARGET_PLATFORM win64)
endif()
if(APPLE)
    set(THIS_PROJECT_TARGET_PLATFORM macos-${CMAKE_OSX_ARCHITECTURES})
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/third_party/")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/config/")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Resources
set(RESOURCES_SOURCE_DIR ${PROJECT_SOURCE_DIR}/resources)

# Stdutils
add_subdirectory(src/stdutils)

# Picross library
add_subdirectory(src/picross)

# Utility library
if(PICROSS_BUILD_GUI OR PICROSS_BUILD_CLI OR PICROSS_BUILD_TESTS)
    add_subdirectory(src/utils)
endif()

# Picross graphical app
if(PICROSS_BUILD_GUI)
    add_subdirectory(src/apps/gui/imgui)
    add_subdirectory(src/apps/gui)
endif()

# Picross CLI
if(PICROSS_BUILD_CLI)
    add_subdirectory(src/apps/cli)
endif()

# Tests
if(PICROSS_BUILD_TESTS)
    add_subdirectory(src/tests/stdutils)
    add_subdirectory(src/tests/picross)
endif()

# Examples
if(PICROSS_BUILD_EXAMPLES)
    add_subdirectory(src/apps/examples)
endif()

install(EXPORT picross_project NAMESPACE picross:: DESTINATION lib/picross)
install(FILES ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION .)

set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE)
include(CPack)
