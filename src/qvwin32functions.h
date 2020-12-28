#ifndef QVWIN32FUNCTIONS_H
#define QVWIN32FUNCTIONS_H

#include "openwith.h"

class QVWin32Functions
{
public:
    static QList<OpenWith::OpenWithItem> getOpenWithItems(const QString &filePath);

    static void openWithAppx(const QString &filePath, const QString &amuid);

    static QString getAumid(const QString &packageFullName);

    static void showOpenWithDialog(const QString &filePath, const QWindow *parent);
};

#endif // QVWIN32FUNCTIONS_H
