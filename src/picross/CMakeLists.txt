#
# Build the library
#

include(compiler_options)

configure_file(src/picross_solver_version.h.in picross_solver_version.h @ONLY)

set(LIB_SOURCES
    src/binomial.cpp
    src/grid.cpp
    src/input_grid.cpp
    src/line.cpp
    src/line_alternatives.cpp
    src/line_constraint.cpp
    src/output_grid.cpp
    src/picross.cpp
    src/picross_io.cpp
    src/picross_stats.cpp
    src/solver.cpp
    src/work_grid.cpp
)

file(GLOB LIB_HEADERS include/picross/*.h src/*.h)

add_library(picross STATIC ${LIB_SOURCES} ${LIB_HEADERS})
add_library(picross::picross ALIAS picross)

target_include_directories(picross
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_warnings(picross ON)

set_property(TARGET picross PROPERTY FOLDER "libraries")

install(TARGETS picross EXPORT picross_project)
install(EXPORT picross_project NAMESPACE picross:: DESTINATION lib/picross)
install(DIRECTORY include/picross DESTINATION include)      # Public headers
install(FILES ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION .)
