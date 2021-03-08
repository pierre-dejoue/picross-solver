#
# Build the CLI application
#

# Dependencies
include(argagg)

set(CLI_SOURCES
    src/observer.cpp
    src/main.cpp
)

file(GLOB CLI_HEADERS src/*.h)

add_executable(picross_solver_cli ${CLI_SOURCES} ${CLI_HEADERS})

target_link_libraries(picross_solver_cli
    PRIVATE
    argagg-lib
    picross
)

install(TARGETS picross_solver_cli)
