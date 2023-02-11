#
# Build the examples
#

include(compiler_options)

set(EXAMPLE_1_SOURCES
    src/example_1.cpp
)

add_executable(example_1 ${EXAMPLE_1_SOURCES})

target_link_libraries(example_1
    PRIVATE
    picross::picross
)

set_target_warnings(example_1 OFF)
