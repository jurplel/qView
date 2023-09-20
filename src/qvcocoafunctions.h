#ifndef QVCOCOAFUNCTIONS_H
#define QVCOCOAFUNCTIONS_H

#include "openwith.h"

#include <QWindow>
#include <QMenu>

class QVCocoaFunctions
{
public:
    static void showMenu(QMenu *menu, const QPoint &point, QWindow *window);

    static void setUserDefaults();

    static void setFullSizeContentView(QWindow *window);

    static bool getTitlebarHidden(QWindow *window);

    static void setTitlebarHidden(QWindow *window, const bool shouldHide);

    static void setVibrancy(bool alwaysDark, QWindow *window);

    static int getObscuredHeight(QWindow *window);

    static void closeWindow(QWindow *window);

    static void setWindowMenu(QMenu *menu);

    static void setAlternates(QMenu *menu, int index0, int index1);

    static void setDockRecents(const QStringList &recentPathsList);

    static QList<OpenWith::OpenWithItem> getOpenWithItems(const QString &filePath);

    static QString deleteFile(const QString &filePath);

    static QByteArray getIccProfileForWindow(const QWindow *window);
};

#endif // QVCOCOAFUNCTIONS_H
