#include "qvlinuxx11functions.h"
#include <QGuiApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QX11Info>
#endif
#include <X11/Xlib.h>
#include <X11/Xatom.h>

namespace X11Helper
{
    Display* getDisplay()
    {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        return QX11Info::display();
#else
        if (const auto x11App = qGuiApp->nativeInterface<QNativeInterface::QX11Application>())
           return x11App->display();
        return nullptr;
#endif
    }
}

QByteArray QVLinuxX11Functions::getIccProfileForWindow(const QWindow *window)
{
    Q_UNUSED(window); // Unused for now; not sure how to handle multiple monitors
    QByteArray result;
    Display *display = X11Helper::getDisplay();
    if (display)
    {
        Atom iccProfileAtom = XInternAtom(display, "_ICC_PROFILE", True);
        if (iccProfileAtom != None)
        {
            Atom type;
            int format;
            unsigned long size;
            unsigned long size_left;
            unsigned char *data;
            int status = XGetWindowProperty(
                display,
                DefaultRootWindow(display),
                iccProfileAtom,
                0,
                INT_MAX,
                False,
                XA_CARDINAL,
                &type,
                &format,
                &size,
                &size_left,
                &data);
            if (status == Success && data)
            {
                result = QByteArray(reinterpret_cast<char*>(data), size);
                XFree(data);
            }
        }
    }
    return result;
}
