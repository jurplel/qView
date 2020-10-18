#ifndef QVCOCOAFUNCTIONS_H
#define QVCOCOAFUNCTIONS_H

#include <QWindow>
#include <QMenu>

class QVCocoaFunctions
{
public:
    static void showMenu(QMenu *menu, const QPoint &point, QWindow *window);

    static void setUserDefaults();

    static void setFullSizeContentView(QWindow *window);

    static void setVibrancy(bool alwaysDark, QWindow *window);

    static int getObscuredHeight(QWindow *window);

    static void closeWindow(QWindow *window);

    static void setWindowMenu(QMenu *menu);

    static void setAlternates(QMenu *menu, int index0, int index1);

    static void setDockRecents(const QStringList &recentPathsList);
};

#endif // QVCOCOAFUNCTIONS_H
