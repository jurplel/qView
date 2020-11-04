TARGET = qView
VERSION = 4.0

QT += core gui network widgets

TEMPLATE = app

# allows use of version variable elsewhere
DEFINES += "VERSION=$$VERSION"

# build folder organization
DESTDIR = bin
OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build
RCC_DIR = build

CONFIG -= debug_and_release debug_and_release_target

# enable c++11
CONFIG += c++11

# Print if this is a debug or release build
CONFIG(debug, debug|release) {
    message("This is a debug build")
} else {
    message("This is a release build")
}

# Check nightly variable
# to use: qmake NIGHTLY=VERSION
!isEmpty(NIGHTLY) {
    message("This is nightly $$NIGHTLY")
    DEFINES += "NIGHTLY=$$NIGHTLY"
}

# Windows specific stuff
win32 {
    QT += svg # needed for including svg support in static build
    RC_ICONS = "dist/win/qView.ico"
    QMAKE_TARGET_COPYRIGHT = "Copyright © 2020 jurplel and qView contributors"
    QMAKE_TARGET_DESCRIPTION = "qView"
}

# macOS specific stuff
macx {
    QT += svg # needed for macdeployqt added qsvg plugin automatically

    # To build without cocoa: qmake CONFIG+=NO_COCOA
    !CONFIG(NO_COCOA) {
        LIBS += -framework Cocoa
        DEFINES += COCOA_LOADED
        message("Linked to cocoa framework")
    }
    QMAKE_TARGET_BUNDLE_PREFIX = "com.interversehq"

    # Special info.plist for qt 5.9 on mac
    equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 10) {
        QMAKE_INFO_PLIST = "dist/mac/Info_legacy.plist"
    } else {
        QMAKE_INFO_PLIST = "dist/mac/Info.plist"
    }

    ICON = "dist/mac/qView.icns"
}

# Stuff for make install
# To use a custom prefix: qmake PREFIX=/usr
# An environment variable will also work: PREFIX=/usr qmake
# You can also use at install time: make install INSTALL_ROOT=/usr but this will not override the prefix, just set where it begins
isEmpty(PREFIX) {
    PREFIX = $$(PREFIX)
}
isEmpty(PREFIX) {
    PREFIX = /usr/local
}

message("Installation prefix is $$PREFIX")

binary.path = $$PREFIX/bin
binary.files = bin/qview
desktop.path = $$PREFIX/share/applications
desktop.files = dist/linux/com.interversehq.qView.desktop
icon16.path = $$PREFIX/share/icons/hicolor/16x16/apps/
icon16.files = dist/linux/hicolor/16x16/apps/com.interversehq.qView.png
icon32.path = $$PREFIX/share/icons/hicolor/32x32/apps/
icon32.files = dist/linux/hicolor/32x32/apps/com.interversehq.qView.png
icon64.path = $$PREFIX/share/icons/hicolor/64x64/apps/
icon64.files = dist/linux/hicolor/64x64/apps/com.interversehq.qView.png
icon128.path = $$PREFIX/share/icons/hicolor/128x128/apps/
icon128.files = dist/linux/hicolor/128x128/apps/com.interversehq.qView.png
icon256.path = $$PREFIX/share/icons/hicolor/256x256/apps/
icon256.files = dist/linux/hicolor/256x256/apps/com.interversehq.qView.png
iconsvg.path = $$PREFIX/share/icons/hicolor/scalable/apps/
iconsvg.files = dist/linux/hicolor/scalable/apps/com.interversehq.qView.svg
iconsym.path = $$PREFIX/share/icons/hicolor/symbolic/apps/
iconsym.files = dist/linux/hicolor/symbolic/apps/com.interversehq.qView-symbolic.svg
license.path = $$PREFIX/share/licenses/qview/
license.files = LICENSE
appstream.path = $$PREFIX/share/metainfo/
appstream.files = dist/linux/com.interversehq.qView.appdata.xml

unix:INSTALLS += binary desktop icon16 icon32 icon64 icon128 icon256 iconsvg iconsym license appstream
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


CONFIG += lrelease embed_translations
TRANSLATIONS += $$files(i18n/qview_*.ts)

RESOURCES += \
    resources/resources.qrc

