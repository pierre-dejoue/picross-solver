#
# Puzzle benchmark
#
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_custom_target(run_bench
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/bench.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Run puzzle benchmark:"
)

set_property(TARGET run_bench PROPERTY FOLDER "bench")
