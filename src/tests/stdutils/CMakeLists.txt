#
# Unit tests
#
include(catch2)

set(UTESTS_SOURCES
    src/test_span.cpp
)

file(GLOB UTESTS_HEADERS src/*.h)

add_executable(stdutils_utests ${UTESTS_SOURCES} ${UTESTS_HEADERS})

target_link_libraries(stdutils_utests
    PRIVATE
    Catch2::Catch2WithMain
    stdutils
)

set_property(TARGET stdutils_utests PROPERTY FOLDER "tests")

add_custom_target(stdutils_run_utests
    $<TARGET_FILE:stdutils_utests> --skip-benchmarks
    COMMENT "Run stdutils library UTests:"
)
