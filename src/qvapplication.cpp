#include "qvapplication.h"
#include "qvoptionsdialog.h"
#include "qvcocoafunctions.h"
#include <QFileOpenEvent>
#include <QMenu>
#include <QSettings>
#include <QMenuBar>
#include <QImageReader>

#include <QDebug>

QVApplication::QVApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    actionManager = new ActionManager(this);
    connect(actionManager, &ActionManager::recentsMenuUpdated, this, &QVApplication::updateDockRecents);

    // Initialize list of supported files and filters
    const auto byteArrayList = QImageReader::supportedImageFormats();
    for (const auto &byteArray : byteArrayList)
    {
        auto fileExtString = QString::fromUtf8(byteArray);
        filterList << "*." + fileExtString;
    }

    auto filterString = tr("Supported Files") + " (";
    for (const auto &filter : qAsConst(filterList))
    {
        filterString += filter + " ";
    }
    filterString.chop(1);
    filterString += ")";

    nameFilterList << filterString;
    nameFilterList << tr("All Files") + " (*)";

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

    QVCocoaFunctions::setUserDefaults();
}

QVApplication::~QVApplication() {
    dockMenu->deleteLater();
}

bool QVApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        auto *openEvent = dynamic_cast<QFileOpenEvent *>(event);
        openFile(getMainWindow(true), openEvent->file());
    }
    return QApplication::event(event);
}


void QVApplication::openFile(MainWindow *window, const QString &file, bool resize)
{
    window->setJustLaunchedWithImage(resize);
    window->openFile(file);
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
    for (const auto &window : qAsConst(lastActiveWindows))
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
    const auto topLevelWidgets = QApplication::topLevelWidgets();
    for (const auto &widget : topLevelWidgets)
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

    const auto dockMenuActions = dockMenuRecentsLibrary->actions();
    for (const auto &action : dockMenuActions)
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

    if (!lastActiveWindows.isEmpty() && window == lastActiveWindows.first())
        return;

    lastActiveWindows.prepend(window);

    if (lastActiveWindows.length() > 5)
        lastActiveWindows.removeLast();
}

void QVApplication::deleteFromLastActiveWindows(MainWindow *window)
{
    if (!window)
        return;

    lastActiveWindows.removeAll(window);
}
