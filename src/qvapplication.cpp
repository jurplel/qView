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
    connect(dockMenu, &QMenu::triggered, [](QAction *triggeredAction){
       qvApp->getActionManager()->actionTriggered(triggeredAction);
    });

    menuBarSuffix.append(actionManager->cloneAction("newwindow"));
    menuBarSuffix.append(actionManager->cloneAction("open"));
    updateDockRecents();

    #ifdef Q_OS_MACX
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
    // Attempt to use the active window
    if (auto *activeWidget = activeWindow())
    {
        while (activeWidget->parentWidget() != nullptr)
        {
            activeWidget = activeWidget->parentWidget();
        }

        if (auto *window = qobject_cast<MainWindow*>(activeWidget))
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

    // If that is not valid, check all windows and use the first valid one
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
    dockMenu->clear();

    foreach (auto action, actionManager->getRecentsActionList())
    {
        if (action->isVisible())
            dockMenu->addAction(action);
    }
    if (!dockMenu->isEmpty())
        dockMenu->addSeparator();

    dockMenu->addActions(menuBarSuffix);
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
