if(TARGET Catch2::Catch2WithMain)
    return()
endif()

message(STATUS "Third-party: catch2")

Include(FetchContent)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.2.0
)

FetchContent_MakeAvailable(catch2)
