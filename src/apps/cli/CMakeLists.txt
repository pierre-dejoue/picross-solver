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
    stdutils
    picross::picross
    picross::utils
    argagg-lib
)

set_target_warnings(picross_solver_cli ON)

install(TARGETS picross_solver_cli)

add_custom_target(picross_cli_version
    $<TARGET_FILE:picross_solver_cli> --version
)

add_custom_target(picross_cli_generate_validation_csv
    cd ${PROJECT_SOURCE_DIR} && sh ./scripts/validation.sh $<TARGET_FILE:picross_solver_cli> ./outputs/validation.csv
)
