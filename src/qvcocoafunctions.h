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
};

#endif // QVCOCOAFUNCTIONS_H
