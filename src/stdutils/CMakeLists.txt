#
# General utilities common to the library and the applications
#

include(compiler_options)

set(LIB_SOURCES
    src/string.cpp
)

file(GLOB LIB_HEADERS include/stdutils/*.h src/*.h)

add_library(stdutils STATIC ${LIB_SOURCES} ${LIB_HEADERS})

target_include_directories(stdutils
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_warnings(stdutils ON)

set_property(TARGET stdutils PROPERTY FOLDER "libraries")

install(TARGETS stdutils EXPORT picross_project)
