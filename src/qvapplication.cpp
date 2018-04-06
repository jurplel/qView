#include "qvapplication.h"
#include <QFileOpenEvent>

QVApplication::QVApplication(int &argc, char **argv) : QApplication(argc, argv)
{

}

bool QVApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
        getMainWindow()->openFile(openEvent->file());
    }
    return QApplication::event(event);
}

MainWindow *QVApplication::getMainWindow()
{
    foreach (QWidget *w, qApp->topLevelWidgets())
        if (MainWindow* mainWin = qobject_cast<MainWindow*>(w))
            return mainWin;
    return nullptr;
}
