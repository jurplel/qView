#-------------------------------------------------
#
# Project created by QtCreator 2018-02-28T00:59:53
#
#-------------------------------------------------

QT += core gui svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qView
VERSION = 0.4 # major.minor
TEMPLATE = app

# allows use of version variable elsewhere
DEFINES += "VERSION=$$VERSION"

# build options
win32:CONFIG += static # build static on windows

# build folder organization
DESTDIR = bin
OBJECTS_DIR = intermediate
MOC_DIR = intermediate
UI_DIR = intermediate
RCC_DIR = intermediate
win32:CONFIG -= debug_and_release debug_and_release_target


# macOS specific stuff
QMAKE_TARGET_BUNDLE_PREFIX = "com.qview"
QMAKE_INFO_PLIST = "Info.plist"
ICON = "src/images/qView.icns"

# Windows specific stuff
RC_ICONS = "src/images/qView.ico"
QMAKE_TARGET_COPYRIGHT = "Copyright © 2018 jurplel and qView contributors"
QMAKE_TARGET_DESCRIPTION = "qView"

# Linux specific stuff
binary.path = /usr/bin
binary.files = bin/qview
desktop.path = /usr/share/applications
desktop.files = qView.desktop

unix:INSTALLS += binary desktop

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
    src/qvaboutdialog.cpp

HEADERS += \
        src/mainwindow.h \
    src/qvgraphicsview.h \
    src/qvoptionsdialog.h \
    src/qvapplication.h \
    src/qvaboutdialog.h

FORMS += \
        src/mainwindow.ui \
    src/qvoptionsdialog.ui \
    src/qvaboutdialog.ui

RESOURCES += \
    src/resources.qrc
