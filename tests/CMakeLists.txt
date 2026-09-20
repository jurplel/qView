# Tests for qView
# This CMakeLists.txt should be included by the parent project

# Find Qt Test component and other dependencies
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Network Widgets Svg Test REQUIRED)

# Get the same source files as the main project
set(QV_APP_SOURCES
    ../src/mainwindow.cpp
    ../src/openwith.cpp
    ../src/qvgraphicsview.cpp
    ../src/qvoptionsdialog.cpp
    ../src/qvapplication.cpp
    ../src/qvaboutdialog.cpp
    ../src/qvrenamedialog.cpp
    ../src/qvwelcomedialog.cpp
    ../src/qvinfodialog.cpp
    ../src/qvimagecore.cpp
    ../src/qvshortcutdialog.cpp
    ../src/actionmanager.cpp
    ../src/settingsmanager.cpp
    ../src/shortcutmanager.cpp
)

# Add platform-specific sources
if(WIN32)
    if(NOT NO_WIN32)
        list(APPEND QV_APP_SOURCES ../src/qvwin32functions.cpp)
    endif()
elseif(APPLE)
    if(NOT NO_COCOA)
        list(APPEND QV_APP_SOURCES ../src/qvcocoafunctions.mm)
    endif()
elseif(UNIX AND NOT APPLE)
    if(NOT NO_X11)
        list(APPEND QV_APP_SOURCES ../src/qvlinuxx11functions.cpp)
    endif()
endif()

# Add updatechecker if enabled
if(NOT QV_DISABLE_ONLINE_VERSION_CHECK)
    list(APPEND QV_APP_SOURCES ../src/updatechecker.cpp)
endif()

# Tests that drive the application need the whole app built alongside them
function(qv_add_app_test target test_name source)
    add_executable(${target} ${source} ${QV_APP_SOURCES})

    # Add include directory for source headers
    target_include_directories(${target} PRIVATE ../src)

    # Add Qt resources
    target_sources(${target} PRIVATE ../resources/resources.qrc)

    # Link to Qt libraries
    target_link_libraries(${target} PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
    )

    # Platform-specific libraries
    if(WIN32 AND NOT NO_WIN32)
        target_compile_definitions(${target} PRIVATE WIN32_LOADED)
    elseif(APPLE AND NOT NO_COCOA)
        target_compile_definitions(${target} PRIVATE COCOA_LOADED)
    endif()

    if(PLATFORM_SPECIFIC_LIBS)
        target_link_libraries(${target} PRIVATE ${PLATFORM_SPECIFIC_LIBS})
    endif()

    # Add compile definitions
    target_compile_definitions(${target} PRIVATE
        "VERSION=${PROJECT_VERSION}"
        QT_DEPRECATED_WARNINGS
        QT_NO_FOREACH
    )

    if(NOT QV_DISABLE_ONLINE_VERSION_CHECK)
        target_compile_definitions(${target} PRIVATE QV_DISABLE_ONLINE_VERSION_CHECK)
    endif()

    add_test(NAME ${test_name} COMMAND ${target})
endfunction()

qv_add_app_test(actionmanagertests ActionManagerTests tst_actionmanagertests.cpp)
qv_add_app_test(graphicsviewtests GraphicsViewTests tst_graphicsviewtests.cpp)

