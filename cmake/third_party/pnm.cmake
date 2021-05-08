if(TARGET pnm)
    return()
endif()

message(STATUS "Third-party: pnm")

include(FetchContent)
FetchContent_Declare(
    pnm
    GIT_REPOSITORY https://github.com/pierre-dejoue/pnm.git
    GIT_TAG 0f0e26912bc4dc4561c8e3f4d3c4bb367f3e9bb2
)
FetchContent_Populate(pnm)

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
