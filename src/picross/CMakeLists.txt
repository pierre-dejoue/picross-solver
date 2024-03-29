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
    src/line_cache.cpp
    src/line_constraint.cpp
    src/line_span.cpp
    src/output_grid.cpp
    src/output_grid_utils.cpp
    src/picross_io.cpp
    src/picross_solver_version.cpp
    src/picross_stats.cpp
    src/solver.cpp
    src/solver_policy.cpp
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

target_link_libraries(picross
    PRIVATE
    stdutils
)

set_target_warnings(picross ON)

set_property(TARGET picross PROPERTY FOLDER "libraries")

install(TARGETS picross EXPORT picross_project)
install(DIRECTORY include/picross DESTINATION include)      # Public headers
