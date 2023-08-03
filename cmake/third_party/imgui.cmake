if(TARGET imgui)
    return()
endif()

message(STATUS "Third-party: imgui")

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.8
)
FetchContent_Populate(imgui)
