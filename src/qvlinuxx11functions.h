#ifndef QVLINUXX11FUNCTIONS_H
#define QVLINUXX11FUNCTIONS_H

#include <QWindow>

class QVLinuxX11Functions
{
public:
    static QByteArray getIccProfileForWindow(const QWindow *window);
};

#endif // QVLINUXX11FUNCTIONS_H

