#
# Puzzle benchmark
#
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_custom_target(run_bench_timing
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/bench.py timing
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Run puzzle benchmark (timing):"
)

add_dependencies(run_bench_timing picross_solver_cli)

set_property(TARGET run_bench_timing PROPERTY FOLDER "bench")
