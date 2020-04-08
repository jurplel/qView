#ifndef QVCOCOAFUNCTIONS_H
#define QVCOCOAFUNCTIONS_H

#include <QMenu>

class QVCocoaFunctions
{
public:
    QVCocoaFunctions();

    static void showMenu(QMenu *menu, QPoint point, QWindow *window);
};

#endif // QVCOCOAFUNCTIONS_H
