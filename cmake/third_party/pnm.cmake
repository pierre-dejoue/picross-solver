if(TARGET pnm)
    return()
endif()

message(STATUS "Third-party: pnm")

include(FetchContent)
FetchContent_Declare(
    pnm
    GIT_REPOSITORY https://github.com/ToruNiina/pnm.git
    GIT_TAG be7ea4f5ad140c4c073e6e8c8575301be257f7aa
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
