if(TARGET pfd)
    return()
endif()

message(STATUS "Third-party: pfd")

include(FetchContent)
FetchContent_Populate(
    pfd
    QUIET
    GIT_REPOSITORY https://github.com/samhocevar/portable-file-dialogs.git
    GIT_TAG 7f852d88a480020d7f91957cbcefe514fc95000c
)

add_library(pfd
    INTERFACE
    ${pfd_SOURCE_DIR}/portable-file-dialogs.h
)

target_include_directories(pfd
    INTERFACE
    ${pfd_SOURCE_DIR}
)

set_property(TARGET pfd PROPERTY FOLDER "third_parties")
