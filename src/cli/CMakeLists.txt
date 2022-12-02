#
# Build the CLI application
#

# Dependencies
include(argagg)

include(compiler_options)

set(CLI_SOURCES
    src/main.cpp
)

file(GLOB CLI_HEADERS src/*.h)

add_executable(picross_solver_cli ${CLI_SOURCES} ${CLI_HEADERS})

target_link_libraries(picross_solver_cli
    PRIVATE
    argagg-lib
    picross
    utils
)

set_target_warnings(picross_solver_cli OFF)

install(TARGETS picross_solver_cli)
