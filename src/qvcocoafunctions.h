#ifndef QVCOCOAFUNCTIONS_H
#define QVCOCOAFUNCTIONS_H

#include "openwith.h"

#include <QWindow>
#include <QMenu>

class QWidget;

class QVCocoaFunctions
{
public:
    static void showMenu(QMenu *menu, const QPoint &point, QWindow *window);

    static void setUserDefaults();

    static void setFullSizeContentView(QWindow *window, const bool enable);

    static void setVibrancy(bool isHidden, bool alwaysDark, QWindow *window);

    static int getObscuredHeight(QWindow *window);

    static int getTitlebarHeight(QWindow *window);

    static bool startSystemMove(QWidget *widget);

    static void performWindowDrag(QWidget *widget);

    static void closeWindow(QWindow *window);

    static void setWindowMenu(QMenu *menu);

    static void setAlternate(QMenu *menu, int index);

    static void setDockRecents(const QStringList &recentPathsList);

    static QList<OpenWith::OpenWithItem> getOpenWithItems(const QString &filePath);

    static QString deleteFile(const QString &filePath);

    static QByteArray getIccProfileForWindow(const QWindow *window);

    static void setMacWindowBlur(const QWindow *window, const bool enable);
};

#endif // QVCOCOAFUNCTIONS_H
