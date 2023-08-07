#
# Unit tests
#
include(catch2)

set(UTESTS_SOURCES
    src/bench_line_alternatives.cpp
    src/bench_solver.cpp
    src/test_binomial.cpp
    src/test_line_alternatives.cpp
    src/test_line_constraint.cpp
    src/test_solver.cpp
    src/test_utils.cpp
)

file(GLOB UTESTS_HEADERS src/*.h)

add_executable(utests_picross ${UTESTS_SOURCES} ${UTESTS_HEADERS})

set_target_warnings(utests_picross ON)

target_include_directories(utests_picross
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../picross/src
)

target_link_libraries(utests_picross
    PRIVATE
    Catch2::Catch2WithMain
    picross::picross
    picross::utils
)

set_property(TARGET utests_picross PROPERTY FOLDER "tests")

add_custom_target(run_utests_picross
    $<TARGET_FILE:utests_picross> --skip-benchmarks
    COMMENT "Run Picross library UTests:"
)
