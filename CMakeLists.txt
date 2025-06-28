cmake_minimum_required(VERSION 3.16)

project(qView VERSION 7.0 LANGUAGES CXX)

# Optional test building
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
endif()

# Set the application type for platform-specific executables
if(WIN32)
    set(EXECUTABLE_TYPE WIN32)
elseif(APPLE)
    set(EXECUTABLE_TYPE MACOSX_BUNDLE)
endif()

# Enable C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt's automatic tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required Qt components
find_package(Qt6 QUIET COMPONENTS Core Gui Network Widgets Svg)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    set(QT_VERSION ${Qt6_VERSION})
    # Find optional tools
    find_package(Qt6 QUIET COMPONENTS LinguistTools)
else()
    set(QT5_FIND_COMPONENTS Core Gui Network Widgets Svg)
    if(UNIX AND NOT APPLE)
        list(APPEND QT5_FIND_COMPONENTS X11Extras)
    endif()
    find_package(Qt5 REQUIRED COMPONENTS ${QT5_FIND_COMPONENTS})
    set(QT_VERSION_MAJOR 5)
    set(QT_VERSION ${Qt5_VERSION})
    # Find optional tools
    find_package(Qt5 QUIET COMPONENTS LinguistTools)
endif()

# Source files
set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/openwith.cpp
    src/qvgraphicsview.cpp
    src/qvoptionsdialog.cpp
    src/qvapplication.cpp
    src/qvaboutdialog.cpp
    src/qvrenamedialog.cpp
    src/qvwelcomedialog.cpp
    src/qvinfodialog.cpp
    src/qvimagecore.cpp
    src/qvshortcutdialog.cpp
    src/actionmanager.cpp
    src/settingsmanager.cpp
    src/shortcutmanager.cpp
)

# Header files
set(PROJECT_HEADERS
    src/mainwindow.h
    src/openwith.h
    src/qvgraphicsview.h
    src/qvoptionsdialog.h
    src/qvapplication.h
    src/qvaboutdialog.h
    src/qvrenamedialog.h
    src/qvwelcomedialog.h
    src/qvinfodialog.h
    src/qvimagecore.h
    src/qvshortcutdialog.h
    src/actionmanager.h
    src/settingsmanager.h
    src/shortcutmanager.h
)

# UI form files
set(PROJECT_FORMS
    src/mainwindow.ui
    src/qvopenwithdialog.ui
    src/qvoptionsdialog.ui
    src/qvaboutdialog.ui
    src/qvwelcomedialog.ui
    src/qvinfodialog.ui
    src/qvshortcutdialog.ui
)

# To disables both manual and automatic checking for updates
option(QV_DISABLE_ONLINE_VERSION_CHECK "Disable online version check" OFF)
if(NOT QV_DISABLE_ONLINE_VERSION_CHECK)
    list(APPEND PROJECT_SOURCES src/updatechecker.cpp)
    list(APPEND PROJECT_HEADERS src/updatechecker.h)
endif()

# Platform-specific sources
# Add source files required for each operating system.
if(WIN32)
    # For Windows, add Win32-specific functions and headers.
    option(NO_WIN32 "Build without Win32 API" OFF)
    if(NOT NO_WIN32)
        list(APPEND PROJECT_SOURCES src/qvwin32functions.cpp)
        list(APPEND PROJECT_HEADERS src/qvwin32functions.h)
    endif()
elseif(APPLE)
    # For macOS, add Cocoa-specific functions and headers.
    option(NO_COCOA "Build without Cocoa framework" OFF)
    if(NOT NO_COCOA)
        list(APPEND PROJECT_SOURCES src/qvcocoafunctions.mm)
        list(APPEND PROJECT_HEADERS src/qvcocoafunctions.h)
    endif()
elseif(UNIX AND NOT APPLE)
    # For Linux, add X11-specific functions and headers.
    option(NO_X11 "Build without X11" OFF)
    if(NOT NO_X11)
        list(APPEND PROJECT_SOURCES src/qvlinuxx11functions.cpp)
        list(APPEND PROJECT_HEADERS src/qvlinuxx11functions.h)
    endif()
endif()

# Add the executable and link Qt libraries
set(PROJECT_RESOURCES resources/resources.qrc)
if(WIN32)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/dist/win/qView.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/qView.rc"
    )
    list(APPEND PROJECT_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/qView.rc")
endif()
add_executable(${PROJECT_NAME} ${EXECUTABLE_TYPE} ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${PROJECT_FORMS} ${PROJECT_RESOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg)

# Set the executable name to lowercase on Linux
if(UNIX AND NOT APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "qview")
endif()

# Add compile definitions based on options
if(NOT QV_DISABLE_ONLINE_VERSION_CHECK)
    target_compile_definitions(${PROJECT_NAME} PRIVATE QV_DISABLE_ONLINE_VERSION_CHECK)
endif()

# Platform-specific configurations
# Apply settings, link libraries, and define properties for each platform.
if(WIN32)
    # Windows specific: link to Win32 libraries, set icon, and define as a Windows executable.
    if(NOT NO_WIN32)
        target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LOADED)
        target_link_libraries(${PROJECT_NAME} PRIVATE shell32 user32 ole32 shlwapi gdi32)
    endif()

    set(ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/dist/win/qView.ico")
elseif(APPLE)
    # macOS specific: link to Cocoa framework and configure the app bundle.
    if(NOT NO_COCOA)
        target_compile_definitions(${PROJECT_NAME} PRIVATE COCOA_LOADED)
        target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa")
    endif()

    # Set the icon based on the Qt version, as per the Qt documentation.
    if(QT_VERSION_MAJOR LESS 6)
        set(MACOSX_BUNDLE_ICON_FILE "qView_legacy.icns")
    else()
        set(MACOSX_BUNDLE_ICON_FILE "qView.icns")
    endif()

    set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/dist/mac/${MACOSX_BUNDLE_ICON_FILE}")
    set_source_files_properties(${app_icon_macos} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(${PROJECT_NAME} PRIVATE ${app_icon_macos})

    # The Info.plist.in file contains placeholders that will be replaced by
    # CMake. We set MACOSX_ICON_FILENAME here so that configure_file can
    # substitute it in the plist.
    set(MACOSX_ICON_FILENAME ${MACOSX_BUNDLE_ICON_FILE})
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/dist/mac/Info.plist.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
        @ONLY
    )

    # Set bundle properties. MACOSX_BUNDLE_INFO_PLIST tells CMake to use our
    # generated Info.plist instead of the default one.
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
    )
elseif(UNIX AND NOT APPLE)
    if(NOT NO_X11)
        find_package(X11 REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})
        if(NOT QT6_FOUND)
            target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::X11Extras)
        endif()
    endif()
endif()

# Compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "VERSION=${PROJECT_VERSION}"
    QT_DEPRECATED_WARNINGS
    QT_NO_FOREACH
)

# Check for nightly builds, which are identified by the NIGHTLY preprocessor definition.
if(DEFINED NIGHTLY)
    message(STATUS "This is nightly ${NIGHTLY}")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "NIGHTLY=\"${NIGHTLY}\"")
endif()

# Handle translations
if(Qt${QT_VERSION_MAJOR}_LinguistTools_FOUND)
    message(STATUS "LinguistTools found, building translations.")
    file(GLOB TRANSLATIONS "i18n/qview_*.ts")
    if(QT_VERSION_MAJOR EQUAL 5)
        # For Qt5, use qt5_add_translation (not qt5_add_translations)
        qt5_add_translation(QM_FILES ${TRANSLATIONS})
        # Add the generated .qm files as resources or install them
        target_sources(${PROJECT_NAME} PRIVATE ${QM_FILES})
    else()
        # For Qt6, use qt_add_translations
        qt_add_translations(${PROJECT_NAME} TS_FILES ${TRANSLATIONS})
    endif()
else()
    message(STATUS "LinguistTools not found, skipping translations.")
endif()

# Installation rules
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES dist/linux/com.interversehq.qView.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(DIRECTORY dist/linux/hicolor/ DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/qview)
install(FILES dist/linux/com.interversehq.qView.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

# Include tests if requested
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
