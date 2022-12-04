#
# Utilities common to GUI and CLI
#

# Dependencies
include(pnm)

set(LIB_SOURCES
    src/bitmap_io.cpp
    src/console_observer.cpp
    src/grid_observer.cpp
    src/picross_file_io.cpp
    src/strings.cpp
)

file(GLOB LIB_HEADERS include/utils/*.h src/*.h)

add_library(utils STATIC ${LIB_SOURCES} ${LIB_HEADERS})
add_library(picross::utils ALIAS utils)

if(MSVC)
    # Correct definition of macro __cplusplus on Visual Studio, required by third party pnm
    # https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
    target_compile_options(utils PRIVATE "/Zc:__cplusplus")
endif()

target_include_directories(utils
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(utils
    PRIVATE
    picross::picross
    pnm++
)

set_property(TARGET utils PROPERTY FOLDER "libraries")
