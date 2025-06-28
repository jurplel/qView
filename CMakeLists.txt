cmake_minimum_required(VERSION 3.16)

project(qView VERSION 7.0 LANGUAGES CXX)

# Enable C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt's automatic tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find either Qt6 or Qt5
find_package(Qt6 COMPONENTS Core Gui Network Widgets Svg LinguistTools)
if (Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt5 COMPONENTS Core Gui Network Widgets Svg LinguistTools REQUIRED)
    set(QT_VERSION_MAJOR 5)
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
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${PROJECT_FORMS} resources/resources.qrc)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg)

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
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE OFF)
    set(CMAKE_RC_ICONS "dist/win/qView.ico")
elif(APPLE)
    # macOS specific: link to Cocoa framework, configure the app bundle, and set the icon.
    if(NOT NO_COCOA)
        target_compile_definitions(${PROJECT_NAME} PRIVATE COCOA_LOADED)
        target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa")
    endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE ON MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/dist/mac/Info.plist")
    # Use a different icon for older Qt versions on macOS.
    if(QT_VERSION_MAJOR LESS 6)
        set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE "dist/mac/qView_legacy.icns")
    else()
        set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE "dist/mac/qView.icns")
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux specific: find and link to the X11 libraries.
    if(NOT NO_X11)
        find_package(X11 REQUIRED)
        target_compile_definitions(${PROJECT_NAME} PRIVATE X11_LOADED)
        target_link_libraries(${PROJECT_NAME} PRIVATE X11::X11)
        # For Qt5, also link to the X11Extras module.
        if(QT_VERSION_MAJOR EQUAL 5)
            find_package(Qt5 COMPONENTS X11Extras REQUIRED)
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

# Handle translations
file(GLOB TRANSLATIONS "i18n/qview_*.ts")
qt_add_translations(${PROJECT_NAME} TS_FILES ${TRANSLATIONS})

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
