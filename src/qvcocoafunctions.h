#ifndef QVCOCOAFUNCTIONS_H
#define QVCOCOAFUNCTIONS_H

#include <QWindow>
#include <QMenu>

class QVCocoaFunctions
{
public:
    enum class VibrancyMode
    {
        none,
        buttonsOnly,
        vibrant,
    };

    static void showMenu(QMenu *menu, const QPoint &point, QWindow *window);

    static void setUserDefaults();

    static void changeTitlebarMode(const VibrancyMode mode, QWindow *window);

    static void closeWindow(QWindow *window);

    static void setWindowMenu(QMenu *menu);

    static void setAlternates(QMenu *menu, int index0, int index1);
};

#endif // QVCOCOAFUNCTIONS_H
