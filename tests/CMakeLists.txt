# Tests for qView
# This CMakeLists.txt should be included by the parent project

# Find Qt Test component and other dependencies
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Network Widgets Svg Test REQUIRED)

# Get the same source files as the main project
set(TEST_SOURCES
    tst_actionmanagertests.cpp
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
        list(APPEND TEST_SOURCES ../src/qvwin32functions.cpp)
    endif()
elseif(APPLE)
    if(NOT NO_COCOA)
        list(APPEND TEST_SOURCES ../src/qvcocoafunctions.mm)
    endif()
elseif(UNIX AND NOT APPLE)
    if(NOT NO_X11)
        list(APPEND TEST_SOURCES ../src/qvlinuxx11functions.cpp)
    endif()
endif()

# Add updatechecker if enabled
if(NOT QV_DISABLE_ONLINE_VERSION_CHECK)
    list(APPEND TEST_SOURCES ../src/updatechecker.cpp)
endif()

# Test executable
add_executable(actionmanagertests ${TEST_SOURCES})

# Add include directory for source headers
target_include_directories(actionmanagertests PRIVATE ../src)

# Add Qt resources
set(TEST_RESOURCES ../resources/resources.qrc)
target_sources(actionmanagertests PRIVATE ${TEST_RESOURCES})

# Link to Qt libraries
target_link_libraries(actionmanagertests PRIVATE 
    Qt${QT_VERSION_MAJOR}::Core 
    Qt${QT_VERSION_MAJOR}::Gui 
    Qt${QT_VERSION_MAJOR}::Network 
    Qt${QT_VERSION_MAJOR}::Widgets 
    Qt${QT_VERSION_MAJOR}::Svg 
    Qt${QT_VERSION_MAJOR}::Test
)

# Platform-specific libraries
if(WIN32 AND NOT NO_WIN32)
    target_compile_definitions(actionmanagertests PRIVATE WIN32_LOADED)
elseif(APPLE AND NOT NO_COCOA)
    target_compile_definitions(actionmanagertests PRIVATE COCOA_LOADED)
endif()

if(PLATFORM_SPECIFIC_LIBS)
    target_link_libraries(actionmanagertests PRIVATE ${PLATFORM_SPECIFIC_LIBS})
endif()

# Add compile definitions
target_compile_definitions(actionmanagertests PRIVATE
    "VERSION=${PROJECT_VERSION}"
    QT_DEPRECATED_WARNINGS
    QT_NO_FOREACH
)

if(NOT QV_DISABLE_ONLINE_VERSION_CHECK)
    target_compile_definitions(actionmanagertests PRIVATE QV_DISABLE_ONLINE_VERSION_CHECK)
endif()

# Add the test
add_test(NAME ActionManagerTests COMMAND actionmanagertests)
