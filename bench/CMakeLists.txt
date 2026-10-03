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

add_executable(phase_driver phase_driver/src/phase_driver.cpp)

target_link_libraries(phase_driver PRIVATE picross::picross)

set_property(TARGET phase_driver PROPERTY FOLDER "bench")

add_custom_target(run_bench_phases
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/bench.py phases
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Run puzzle benchmark (phases):"
)

add_dependencies(run_bench_phases phase_driver)

set_property(TARGET run_bench_phases PROPERTY FOLDER "bench")
