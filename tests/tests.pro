QT += core testlib gui network widgets

macx:LIBS += -framework Cocoa

VERSION = 1.0
DEFINES += "VERSION=$$VERSION"

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES +=  tst_actionmanagertests.cpp

INCLUDEPATH += ../src
include( ../src/src.pri )

SOURCES -= $$absolute_path(../src/main.cpp)
