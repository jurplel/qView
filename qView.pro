QT += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qView
VERSION = 3.0 # major.minor
TEMPLATE = app

# allows use of version variable elsewhere
DEFINES += "VERSION=$$VERSION"

# build folder organization
DESTDIR = bin
OBJECTS_DIR = intermediate
MOC_DIR = intermediate
UI_DIR = intermediate
RCC_DIR = intermediate
CONFIG -= debug_and_release debug_and_release_target

# enable c++11
CONFIG += c++11

# Windows specific stuff
win32:QT += svg # needed for including svg support in static build
win32:CONFIG += static
RC_ICONS = "dist/win/qView.ico"
QMAKE_TARGET_COPYRIGHT = "Copyright � 2020 jurplel and qView contributors"
QMAKE_TARGET_DESCRIPTION = "qView"

# macOS specific stuff
macx:QT += svg # needed for macdeployqt added qsvg plugin automatically
macx:!CONFIG(NO_COCOA) {
    LIBS += -framework Cocoa
    DEFINES += COCOA_LOADED
}
QMAKE_TARGET_BUNDLE_PREFIX = "com.qview"

equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 10) {
    QMAKE_INFO_PLIST = "dist/mac/Info_legacy.plist"
} else {
    QMAKE_INFO_PLIST = "dist/mac/Info.plist"
}


ICON = "dist/mac/qView.icns"

# Linux specific stuff
binary.path = /usr/bin
binary.files = bin/qview
desktop.path = /usr/share/applications
desktop.files = dist/linux/qView.desktop
icon16.path = /usr/share/icons/hicolor/16x16/apps/
icon16.files = dist/linux/hicolor/16x16/apps/qview.png
icon32.path = /usr/share/icons/hicolor/32x32/apps/
icon32.files = dist/linux/hicolor/32x32/apps/qview.png
icon64.path = /usr/share/icons/hicolor/64x64/apps/
icon64.files = dist/linux/hicolor/64x64/apps/qview.png
icon128.path = /usr/share/icons/hicolor/128x128/apps/
icon128.files = dist/linux/hicolor/128x128/apps/qview.png
icon256.path = /usr/share/icons/hicolor/256x256/apps/
icon256.files = dist/linux/hicolor/256x256/apps/qview.png
iconsvg.path = /usr/share/icons/hicolor/scalable/apps/
iconsvg.files = dist/linux/hicolor/scalable/apps/qview.svg
iconsym.path = /usr/share/icons/hicolor/symbolic/apps/
iconsym.files = dist/linux/hicolor/symbolic/apps/qview-symbolic.svg
license.path = /usr/share/licenses/qview/
license.files = LICENSE
appstream.path = /usr/share/metainfo/
appstream.files = dist/linux/qview.appdata.xml

unix:!macx:INSTALLS += binary desktop icon16 icon32 icon64 icon128 icon256 iconsvg license appstream
unix:!macx:TARGET = qview

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Ban usage of Qt's built in foreach utility for better code style
DEFINES += QT_NO_FOREACH

include(src/src.pri)

RESOURCES += \
    resources.qrc
