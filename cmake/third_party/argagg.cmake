if(TARGET argagg)
    return()
endif()

message(STATUS "Third-party: argagg")

include(FetchContent)
FetchContent_Declare(
    argagg
    GIT_REPOSITORY https://github.com/vietjtnguyen/argagg.git
    GIT_TAG 0.4.7
)
FetchContent_Populate(argagg)

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
