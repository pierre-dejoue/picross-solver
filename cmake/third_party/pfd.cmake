if(TARGET pfd)
    return()
endif()

message(STATUS "Third-party: pfd")

include(FetchContent)
FetchContent_Declare(
    pfd
    GIT_REPOSITORY https://github.com/samhocevar/portable-file-dialogs.git
    GIT_TAG b8ed26a24b2a2de189d379f907ddc93e071cebbc
)
FetchContent_MakeAvailable(pfd)

# To make it visible in the IDE
add_custom_target(pfd SOURCES
    ${pfd_SOURCE_DIR}/portable-file-dialogs.h
)

set_property(TARGET pfd PROPERTY FOLDER "third_parties")
