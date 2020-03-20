#include "qvapplication.h"
#include "qvoptionsdialog.h"
#include <QFileOpenEvent>
#include <QMenu>
#include <QSettings>
#include <QMenuBar>

#include <QDebug>

QVApplication::QVApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    actionManager = new ActionManager(this);
    connect(actionManager, &ActionManager::recentsMenuUpdated, this, &QVApplication::updateDockRecents);

    //don't even try to show menu icons on mac or windows
    #if defined(Q_OS_MACX) || defined(Q_OS_WIN)
    setAttribute(Qt::AA_DontShowIconsInMenus);
    #endif

    dockMenu = new QMenu();

    #ifdef Q_OS_MACX
    dockMenu->setAsDockMenu();
    setQuitOnLastWindowClosed(false);
    #endif

    menuBar = actionManager->buildMenuBar();

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
    MainWindow *w = getEmptyMainWindow();

    if (!w)
        return;

    w->pickFile();
}

void QVApplication::openFile(const QString &file, bool resize)
{
    MainWindow *w = getEmptyMainWindow();

    if (!w)
        return;

    w->setJustLaunchedWithImage(resize);
    w->openFile(file);
}

MainWindow *QVApplication::newWindow(const QString &fileToOpen)
{
    auto *w = new MainWindow();
    w->show();

    if (!fileToOpen.isEmpty())
        w->openFile(fileToOpen);

    return w;
}

MainWindow *QVApplication::getCurrentMainWindow()
{
    // Get active window
    if (auto *activeWidget = activeWindow())
    {
        while (activeWidget->parentWidget() != nullptr)
        {
            activeWidget = activeWidget->parentWidget();
        }

        if (auto *window = qobject_cast<MainWindow*>(activeWidget))
        {
            return window;
        }
    }

    // If there are no valid windows, make a new one.
    auto *window = newWindow();

    return window;
}

MainWindow *QVApplication::getEmptyMainWindow()
{
    // Attempt to use the active window
    if (auto *window = getCurrentMainWindow())
    {
        if (!window->getIsPixmapLoaded())
        {
            return window;
        }
    }

    // If that is not valid, check all windows and use the first valid one
    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        if (auto *window = qobject_cast<MainWindow*>(widget))
        {
            if (!window->getIsPixmapLoaded())
            {
                return window;
            }
        }
    }

    // If there are no valid ones, make a new one.
    auto *window = newWindow();

    return window;
}

void QVApplication::updateDockRecents()
{
    // This entire function is only necessary because invisible actions do not
    // disappear in mac's dock menu
    dockMenu->clear();

    foreach (auto action, actionManager->getRecentsActionList())
    {
        if (action->isVisible())
            dockMenu->addAction(action);
    }
    if (!dockMenu->isEmpty())
        dockMenu->addSeparator();

    dockMenu->addAction(actionManager->getAction("newwindow"));
    dockMenu->addAction(actionManager->getAction("open"));
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
