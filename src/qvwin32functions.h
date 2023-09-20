#ifndef QVWIN32FUNCTIONS_H
#define QVWIN32FUNCTIONS_H

#include "openwith.h"

#include <qfileiconprovider.h>

class QVWin32Functions
{
public:
    static bool getTitlebarHidden(QWindow *window);

    static void setTitlebarHidden(QWindow *window, const bool shouldHide);

    static QList<OpenWith::OpenWithItem> getOpenWithItems(const QString &filePath);

    static void openWithInvokeAssocHandler(const QString &filePath, void *winAssocHandler);

    static void showOpenWithDialog(const QString &filePath, const QWindow *parent);

    static QByteArray getIccProfileForWindow(const QWindow *window);
};

#endif // QVWIN32FUNCTIONS_H
