#-------------------------------------------------
#
# Project created by QtCreator 2018-02-28T00:59:53
#
#-------------------------------------------------

QT += core gui svg network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qView
VERSION = 1.0 # major.minor
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


# Windows specific stuff
win32:CONFIG += static
RC_ICONS = "src/images/qView.ico"
QMAKE_TARGET_COPYRIGHT = "Copyright © 2018 jurplel and qView contributors"
QMAKE_TARGET_DESCRIPTION = "qView"

# macOS specific stuff
QMAKE_TARGET_BUNDLE_PREFIX = "com.qview"
QMAKE_INFO_PLIST = "Info.plist"
ICON = "src/images/qView.icns"

# Linux specific stuff
binary.path = /usr/bin
binary.files = bin/qview
desktop.path = /usr/share/applications
desktop.files = qView.desktop
icon16.path = /usr/share/icons/hicolor/16x16/apps/
icon16.files = src/images/linux/hicolor/16x16/apps/qview.png
icon32.path = /usr/share/icons/hicolor/32x32/apps/
icon32.files = src/images/linux/hicolor/32x32/apps/qview.png
icon64.path = /usr/share/icons/hicolor/64x64/apps/
icon64.files = src/images/linux/hicolor/64x64/apps/qview.png
icon128.path = /usr/share/icons/hicolor/128x128/apps/
icon128.files = src/images/linux/hicolor/128x128/apps/qview.png
icon256.path = /usr/share/icons/hicolor/256x256/apps/
icon256.files = src/images/linux/hicolor/256x256/apps/qview.png
iconsvg.path = /usr/share/icons/hicolor/scalable/apps/
iconsvg.files = src/images/linux/hicolor/scalable/apps/qview.svg

unix:!macx:INSTALLS += binary desktop icon16 icon32 icon64 icon128 icon256 iconsvg
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


SOURCES += \
        src/main.cpp \
        src/mainwindow.cpp \
    src/qvgraphicsview.cpp \
    src/qvoptionsdialog.cpp \
    src/qvapplication.cpp \
    src/qvaboutdialog.cpp \
    src/qvwelcomedialog.cpp \
    src/qvinfodialog.cpp

HEADERS += \
        src/mainwindow.h \
    src/qvgraphicsview.h \
    src/qvoptionsdialog.h \
    src/qvapplication.h \
    src/qvaboutdialog.h \
    src/qvwelcomedialog.h \
    src/qvinfodialog.h

FORMS += \
        src/mainwindow.ui \
    src/qvoptionsdialog.ui \
    src/qvaboutdialog.ui \
    src/qvwelcomedialog.ui \
    src/qvinfodialog.ui

RESOURCES += \
    src/resources.qrc
