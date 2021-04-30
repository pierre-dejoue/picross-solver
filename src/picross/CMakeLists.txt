#
# Build the library
#

set(LIB_SOURCES
    src/binomial.cpp
    src/line.cpp
    src/line_constraint.cpp
    src/output_grid.cpp
    src/picross.cpp
    src/picross_io.cpp
    src/solver.cpp
    src/work_grid.cpp
)

file(GLOB LIB_HEADERS include/picross/*.h src/*.h)

add_library(picross STATIC ${LIB_SOURCES} ${LIB_HEADERS})

target_include_directories(picross
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_property(TARGET picross PROPERTY FOLDER "libraries")

install(TARGETS picross)
install(DIRECTORY include/picross DESTINATION include)      # Public headers
install(FILES ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION .)
