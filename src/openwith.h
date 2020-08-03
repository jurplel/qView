#ifndef OPENWITH_H
#define OPENWITH_H

#include <QIcon>

class OpenWith
{
public:
    struct OpenWithItem {
        QIcon icon;
        QString name;
        QString exec;
    };

    static const QList<OpenWithItem> getOpenWithItems(const QString &mimeName);

    static void showOpenWithDialog();
};

#endif // OPENWITH_H
