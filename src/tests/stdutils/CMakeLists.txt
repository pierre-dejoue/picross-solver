#
# Unit tests
#
include(catch2)

set(UTESTS_SOURCES
    src/test_io.cpp
    src/test_span.cpp
    src/test_string.cpp
)

file(GLOB UTESTS_HEADERS src/*.h)

add_executable(utests_stdutils ${UTESTS_SOURCES} ${UTESTS_HEADERS})

set_target_warnings(utests_stdutils ON)

target_link_libraries(utests_stdutils
    PRIVATE
    Catch2::Catch2WithMain
    stdutils
)

set_property(TARGET utests_stdutils PROPERTY FOLDER "tests")

add_custom_target(run_utests_stdutils
    $<TARGET_FILE:utests_stdutils> --skip-benchmarks
    COMMENT "Run stdutils library UTests:"
)
