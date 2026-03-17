#
# Build the graphical application
#

# Dependencies
find_package(OpenGL REQUIRED)       # Necessary because we do not use a loader (such as gl3w or glad)
include(pfd)

set(GUI_SOURCES
    src/about_window.cpp
    src/bitmap_io_raw.cpp
    src/bitmap_to_glfw_image.cpp
    src/draw_grid.cpp
    src/embedded_file.cpp
    src/err_window.cpp
    src/glfw_context.cpp
    src/imgui_helpers.cpp
    src/goal_window.cpp
    src/grid_info.cpp
    src/grid_window.cpp
    src/logo.cpp
    src/logo_file.cpp
    src/main.cpp
    src/picross_file.cpp
    src/settings.cpp
    src/settings_window.cpp
    src/style.cpp
    src/window_icon.cpp
)

if(NOT APPLE)
    list(APPEND GUI_SOURCES
        src/window_icon_files.cpp
    )
endif()

file(GLOB GUI_HEADERS src/*.h)


set(THIS_APPLICATION_ID                 ${THIS_PROJECT_ID})
set(THIS_APPLICATION_NAME               ${THIS_PROJECT_NAME})
set(THIS_APPLICATION_LICENSE_FILE       ${PROJECT_SOURCE_DIR}/LICENSE)

# This identifier is used in the macOS bundle and it should only contain alphanumeric characters (A-Z, a-z, 0-9), hyphens (-), and periods (.)
set(THIS_APPLICATION_MACOSX_BUNDLE_ID   ${THIS_PROJECT_DOMAIN_TLD}.${THIS_PROJECT_DOMAIN_ID}.${THIS_APPLICATION_ID})

set(THIS_APPLICATION_MACOSX_ICON        ${PROJECT_SOURCE_DIR}/resources/app.icns)

if(MSVC)
    configure_file(src/icon.rc.in src/icon.rc @ONLY)
    set(APP_ICON ${CMAKE_CURRENT_BINARY_DIR}/src/icon.rc)
else()
    set(APP_ICON "")
endif()

if(APPLE)
    add_executable(picross_solver
        MACOSX_BUNDLE
        ${GUI_SOURCES}
        ${GUI_HEADERS}
        ${THIS_APPLICATION_MACOSX_ICON}
        ${THIS_APPLICATION_LICENSE_FILE}
    )
else()
    add_executable(picross_solver
        ${GUI_SOURCES}
        ${GUI_HEADERS}
        ${APP_ICON}
    )
endif()

if(APPLE)
    # OpenGL API is deprecated since macOS 10.14
    target_compile_definitions(picross_solver
        PUBLIC
        GL_SILENCE_DEPRECATION
    )
endif()

target_include_directories(picross_solver
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_warnings(picross_solver ON)

target_link_libraries(picross_solver
    PRIVATE
    stdutils
    picross::picross
    picross::utils
    glfw
    imgui
    OpenGL::GL
    pfd
)

set(CPACK_PACKAGE_NAME      ${THIS_APPLICATION_ID})
set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CMAKE_PROJECT_VERSION}-${THIS_PROJECT_TARGET_PLATFORM})

if(APPLE)
    #
    # Packaging for macOS. We bundle the app with CMake, and create a DMG image with an external script
    #
    set_source_files_properties(${THIS_APPLICATION_MACOSX_ICON}     PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set_source_files_properties(${THIS_APPLICATION_LICENSE_FILE}    PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set_target_properties(picross_solver PROPERTIES
        BUNDLE True
        OUTPUT_NAME                         ${THIS_APPLICATION_NAME}
        MACOSX_BUNDLE_GUI_IDENTIFIER        ${THIS_APPLICATION_MACOSX_BUNDLE_ID}
        MACOSX_BUNDLE_BUNDLE_NAME           ${THIS_APPLICATION_NAME}
        MACOSX_BUNDLE_BUNDLE_VERSION        ${CMAKE_PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING  ${CMAKE_PROJECT_VERSION}
        MACOSX_BUNDLE_COPYRIGHT             ${THIS_PROJECT_COPYRIGHT}
        MACOSX_BUNDLE_ICON_FILE             app.icns
        MACOSX_BUNDLE_INFO_PLIST            ${PROJECT_SOURCE_DIR}/src/utils/macos/bundle_info_template.plist.in
    )
    set(CPACK_PACKAGE_ICON "${THIS_APPLICATION_MACOSX_ICON}")

    install(TARGETS picross_solver
        BUNDLE DESTINATION .
    )
else()
    install(TARGETS picross_solver)
endif()
