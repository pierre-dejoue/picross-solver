#
# Build the library
#

include(compiler_options)

set(LIB_SOURCES
    src/binomial.cpp
    src/line.cpp
    src/line_constraint.cpp
    src/output_grid.cpp
    src/picross.cpp
    src/picross_io.cpp
    src/solver.cpp
    src/work_grid.cpp
)

file(GLOB LIB_HEADERS include/picross/*.h src/*.h)

add_library(picross STATIC ${LIB_SOURCES} ${LIB_HEADERS})

configure_file(src/picross_solver_version.h.in picross_solver_version.h @ONLY)

target_include_directories(picross
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_warnings(picross ON)

set_property(TARGET picross PROPERTY FOLDER "libraries")

install(TARGETS picross)
install(DIRECTORY include/picross DESTINATION include)      # Public headers
install(FILES ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION .)
