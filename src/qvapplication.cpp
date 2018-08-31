#include "qvapplication.h"
#include <QFileOpenEvent>

QVApplication::QVApplication(int &argc, char **argv) : QApplication(argc, argv)
{

}

bool QVApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
        openFile(openEvent->file());
    }
    return QApplication::event(event);
}

void QVApplication::openFile(QString file)
{
    if (!getMainWindow()->getIsPixmapLoaded())
    {
        getMainWindow()->openFile(file);
    }
    else
    {
        MainWindow *w = newWindow();
        w->openFile(file);
    }
}

MainWindow *QVApplication::newWindow()
{
    MainWindow *w = new MainWindow();
    w->show();
    w->setAttribute(Qt::WA_DeleteOnClose);
    return w;
}

MainWindow *QVApplication::getMainWindow()
{
    foreach (QWidget *w, qApp->topLevelWidgets())
        if (MainWindow* mainWin = qobject_cast<MainWindow*>(w))
            return mainWin;
    return nullptr;
}
