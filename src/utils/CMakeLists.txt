#
# Utilities common to GUI and CLI
#

add_library(utils INTERFACE)

target_include_directories(utils
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

#set_property(TARGET utils PROPERTY FOLDER "libraries")
