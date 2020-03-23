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
    #if defined Q_OS_MACOS || defined Q_OS_WIN
    setAttribute(Qt::AA_DontShowIconsInMenus);
    #endif

    dockMenu = new QMenu();
    connect(dockMenu, &QMenu::triggered, [](QAction *triggeredAction){
       qvApp->getActionManager()->actionTriggered(triggeredAction);
    });

    dockMenuSuffix.append(actionManager->cloneAction("newwindow"));
    dockMenuSuffix.append(actionManager->cloneAction("open"));

    dockMenuRecentsLibrary = nullptr;
    dockMenuRecentsLibrary = actionManager->buildRecentsMenu(false);
    actionManager->updateRecentsMenu();

    #ifdef Q_OS_MACOS
    dockMenu->setAsDockMenu();
    setQuitOnLastWindowClosed(false);
    #endif

    menuBar = actionManager->buildMenuBar();
    connect(menuBar, &QMenuBar::triggered, [](QAction *triggeredAction){
        qvApp->getActionManager()->actionTriggered(triggeredAction);
    });
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


void QVApplication::openFile(const QString &file, bool resize)
{
    if (auto *window = getMainWindow(true))
    {
        window->setJustLaunchedWithImage(resize);
        window->openFile(file);
    }
}

MainWindow *QVApplication::newWindow()
{
    auto *w = new MainWindow();
    w->show();

    return w;
}

MainWindow *QVApplication::getMainWindow(bool shouldBeEmpty)
{
    // Attempt to use from list of last active windows
    foreach (auto *window, lastActiveWindows)
    {
        if (!window)
            continue;

        if (shouldBeEmpty)
        {
            if (!window->getIsPixmapLoaded())
            {
                return window;
            }
        }
        else
        {
            return window;
        }
    }

    // If none of those are valid, scan the list for any existing MainWindow
    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        if (auto *window = qobject_cast<MainWindow*>(widget))
        {
            if (shouldBeEmpty)
            {
                if (!window->getIsPixmapLoaded())
                {
                    return window;
                }
            }
            else
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
    if (!dockMenuRecentsLibrary)
        return;

    dockMenu->clear();

    foreach (auto action, dockMenuRecentsLibrary->actions())
    {
        if (action->isVisible())
            dockMenu->addAction(action);
    }
    if (!dockMenu->isEmpty())
        dockMenu->addSeparator();

    dockMenu->addActions(dockMenuSuffix);
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

void QVApplication::addToLastActiveWindows(MainWindow *window)
{
    if (!window)
        return;

    lastActiveWindows.prepend(window);

    if (lastActiveWindows.length() > 5)
        lastActiveWindows.removeLast();
}

void QVApplication::deleteFromLastActiveWindows(MainWindow *window)
{
    if (!window)
        return;

    lastActiveWindows.removeOne(window);
}
