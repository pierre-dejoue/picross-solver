if(TARGET argagg)
    return()
endif()

message(STATUS "Third-party: argagg")

include(FetchContent)
FetchContent_Populate(
    argagg
    QUIET
    GIT_REPOSITORY https://github.com/vietjtnguyen/argagg.git
    GIT_TAG 6cd4a64ae09795c178d9ccba6d763c5fc9dcb72a
)

add_library(argagg-lib INTERFACE)

target_include_directories(argagg-lib
    INTERFACE
    ${argagg_SOURCE_DIR}/include
)

# To make it visible in the IDE
add_custom_target(argagg SOURCES
    ${argagg_SOURCE_DIR}/include/argagg/argagg.hpp
)

set_property(TARGET argagg PROPERTY FOLDER "third_parties")
