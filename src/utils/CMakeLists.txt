#
# Utilities common to GUI and CLI
#

set(LIB_SOURCES
    src/console_observer.cpp
    src/grid_observer.cpp
)

file(GLOB LIB_HEADERS include/*.h src/*.h)

add_library(utils STATIC ${LIB_SOURCES} ${LIB_HEADERS})

target_include_directories(utils
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(utils
    PRIVATE
    picross
)

set_property(TARGET utils PROPERTY FOLDER "libraries")
