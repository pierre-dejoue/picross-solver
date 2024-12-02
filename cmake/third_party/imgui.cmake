if(TARGET imgui)
    return()
endif()

message(STATUS "Third-party: imgui")

include(FetchContent)
FetchContent_Populate(
    imgui
    QUIET
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.0
)
