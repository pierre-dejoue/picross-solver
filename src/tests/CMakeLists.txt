#
# Unit tests
#
include(catch2)

set(UTESTS_SOURCES
    src/test_solver.cpp
)

file(GLOB UTESTS_HEADERS src/*.h)

add_executable(picross_utests ${UTESTS_SOURCES} ${UTESTS_HEADERS})

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
