#
# Build the CLI application
#

set(CLI_SOURCES
    src/main.cpp
)

file(GLOB CLI_HEADERS src/*.h)

add_executable(picross_solver ${CLI_SOURCES} ${CLI_HEADERS})

target_link_libraries(picross_solver PRIVATE picross)

install(TARGETS picross_solver)
