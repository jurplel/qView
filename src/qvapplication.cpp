#include "qvapplication.h"
#include <QFileOpenEvent>
#include <QMenu>
#include <QSettings>

QVApplication::QVApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    //don't even try to show menu icons on mac or windows
    #if defined(Q_OS_MACX) || defined(Q_OS_WIN)
    setAttribute(Qt::AA_DontShowIconsInMenus);
    #endif


    dockMenu = new QMenu();
    #ifdef Q_OS_MACX
    dockMenu->setAsDockMenu();
    #endif
    updateDockRecents();
}

QVApplication::~QVApplication() {
    dockMenu->deleteLater();
}

bool QVApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        auto *openEvent = dynamic_cast<QFileOpenEvent *>(event);
        openFile(openEvent->file());
    }
    return QApplication::event(event);
}

void QVApplication::pickFile()
{
    MainWindow *w = getMainWindow();

    if (!w)
        return;

    w->pickFile();
}

void QVApplication::openFile(const QString &file, bool resize)
{
    MainWindow *w = getMainWindow();

    if (!w)
        return;

    w->setJustLaunchedWithImage(resize);
    w->openFile(file);
}

MainWindow *QVApplication::newWindow()
{
    auto *w = new MainWindow();
    w->show();
    return w;
}

MainWindow *QVApplication::getMainWindow()
{
    MainWindow *w = nullptr;
    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        if (auto *mainWin = qobject_cast<MainWindow*>(widget))
        {
            if (!mainWin->getIsPixmapLoaded())
            {
                w = mainWin;
            }
        }
    }

    if (!w)
        w = newWindow();

    return w;
}

void QVApplication::updateDockRecents()
{
    QSettings settings;
    settings.beginGroup("recents");
    QVariantList recentFiles = settings.value("recentFiles").value<QVariantList>();

    dockMenu->clear();
    for (int i = 0; i <= 9; i++)
    {
        if (i < recentFiles.size())
        {
            auto *action = new QAction(recentFiles[i].toList().first().toString());
            connect(action, &QAction::triggered, [recentFiles, i]{
               openFile(recentFiles[i].toList().last().toString(), false);
            });
            dockMenu->addAction(action);
        }
    }
    auto *newWindowAction = new QAction(tr("New Window"));
    connect(newWindowAction, &QAction::triggered, []{
        newWindow();
    });
    auto *openAction = new QAction(tr("Open..."));
    connect(openAction, &QAction::triggered, []{
        pickFile();
    });
    dockMenu->addSeparator();
    dockMenu->addAction(newWindowAction);
    dockMenu->addAction(openAction);
}

qint64 QVApplication::getPreviouslyRecordedFileSize(const QString &fileName)
{
    auto previouslyRecordedFileSizePtr = previouslyRecordedFileSizes.object(fileName);
    qint64 previouslyRecordedFileSize = 0;

    if (previouslyRecordedFileSizePtr)
        previouslyRecordedFileSize = *previouslyRecordedFileSizePtr;

    return previouslyRecordedFileSize;
}

void QVApplication::setPreviouslyRecordedFileSize(const QString &fileName, long long *fileSize)
{
    previouslyRecordedFileSizes.insert(fileName, fileSize);
}
