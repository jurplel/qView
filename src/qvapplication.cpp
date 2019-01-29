#include "qvapplication.h"
#include <QFileOpenEvent>

QVApplication::QVApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    //don't even try to show menu icons on mac or windows
    #if defined(Q_OS_MACX) || defined(Q_OS_WIN)
    setAttribute(Qt::AA_DontShowIconsInMenus);
    #endif
}

bool QVApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        auto *openEvent = dynamic_cast<QFileOpenEvent *>(event);
        openFile(openEvent->file());
    }
    return QApplication::event(event);
}

void QVApplication::openFile(const QString &file)
{
    MainWindow *w = getMainWindow();

    if (!w)
        w = newWindow();

    w->setJustLaunchedWithImage(true);
    w->openFile(file);
}

MainWindow *QVApplication::newWindow()
{
    auto *w = new MainWindow();
    w->show();
    w->setAttribute(Qt::WA_DeleteOnClose);
    return w;
}

MainWindow *QVApplication::getMainWindow()
{
    foreach (QWidget *w, QApplication::topLevelWidgets())
        if (auto mainWin = qobject_cast<MainWindow*>(w))
            if (!mainWin->getIsPixmapLoaded())
                return mainWin;
    return nullptr;
}
