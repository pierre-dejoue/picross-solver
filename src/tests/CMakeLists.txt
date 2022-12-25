#
# Unit tests
#
include(catch2)

set(UTESTS_SOURCES
    src/bench_solver.cpp
    src/test_binomial.cpp
    src/test_line_alternatives.cpp
    src/test_line_constraint.cpp
    src/test_solver.cpp
)

file(GLOB UTESTS_HEADERS src/*.h)

add_executable(picross_utests ${UTESTS_SOURCES} ${UTESTS_HEADERS})

target_include_directories(picross_utests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../picross/src
)

target_link_libraries(picross_utests
    PRIVATE
    Catch2::Catch2WithMain
    picross::picross
    picross::utils
)

set_property(TARGET picross_utests PROPERTY FOLDER "tests")

add_custom_target(picross_run_utests
    $<TARGET_FILE:picross_utests> --skip-benchmarks
    COMMENT "Run Picross library UTests:"
)
