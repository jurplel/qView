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

    QVCocoaFunctions();

    static void showMenu(QMenu *menu, QPoint point, QWindow *window);

    static void changeTitlebarMode(const VibrancyMode mode, QWindow *window);
};

#endif // QVCOCOAFUNCTIONS_H
