if(TARGET pnm)
    return()
endif()

message(STATUS "Third-party: pnm")

include(FetchContent)
FetchContent_Declare(
    pnm
    GIT_REPOSITORY https://github.com/pierre-dejoue/pnm.git
    GIT_TAG 0a689ee37ef7ba0b9a30dfa79486dbcb5e12582f
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
