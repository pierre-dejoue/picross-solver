if(TARGET pnm)
    return()
endif()

message(STATUS "Third-party: pnm")

include(FetchContent)
FetchContent_Populate(
    pnm
    QUIET
    GIT_REPOSITORY https://github.com/ToruNiina/pnm.git
    GIT_TAG v1.0.1
)

add_library(pnm++ INTERFACE)

target_include_directories(pnm++
    INTERFACE
    ${pnm_SOURCE_DIR}
)

# To make it visible in the IDE
add_custom_target(pnm SOURCES
    ${pnm_SOURCE_DIR}/pnm.hpp
)

set_property(TARGET pnm PROPERTY FOLDER "third_parties")
