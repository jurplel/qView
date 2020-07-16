SOURCES += \
    $$PWD/main.cpp \
    $$PWD/mainwindow.cpp \
    $$PWD/qvgraphicsview.cpp \
    $$PWD/qvoptionsdialog.cpp \
    $$PWD/qvapplication.cpp \
    $$PWD/qvaboutdialog.cpp \
    $$PWD/qvwelcomedialog.cpp \
    $$PWD/qvinfodialog.cpp \
    $$PWD/qvimagecore.cpp \
    $$PWD/qvshortcutdialog.cpp \
    $$PWD/actionmanager.cpp \
    $$PWD/settingsmanager.cpp \
    $$PWD/shortcutmanager.cpp \
    $$PWD/updatechecker.cpp

macx:SOURCES += $$PWD/qvcocoafunctions.mm

HEADERS += \
    $$PWD/mainwindow.h \
    $$PWD/qvgraphicsview.h \
    $$PWD/qvoptionsdialog.h \
    $$PWD/qvapplication.h \
    $$PWD/qvaboutdialog.h \
    $$PWD/qvwelcomedialog.h \
    $$PWD/qvinfodialog.h \
    $$PWD/qvimagecore.h \
    $$PWD/qvshortcutdialog.h \
    $$PWD/actionmanager.h \
    $$PWD/settingsmanager.h \
    $$PWD/shortcutmanager.h \
    $$PWD/updatechecker.h

macx:HEADERS += $$PWD/qvcocoafunctions.h

FORMS += \
        $$PWD/mainwindow.ui \
    $$PWD/qvoptionsdialog.ui \
    $$PWD/qvaboutdialog.ui \
    $$PWD/qvwelcomedialog.ui \
    $$PWD/qvinfodialog.ui \
    $$PWD/qvshortcutdialog.ui
