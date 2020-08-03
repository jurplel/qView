#include "actionmanager.h"
#include "qvapplication.h"
#include "qvcocoafunctions.h"

#include <QSettings>
#include <QMimeDatabase>
#include <QFileIconProvider>

ActionManager::ActionManager(QObject *parent) : QObject(parent)
{
    isSaveRecentsEnabled = true;
    recentsListMaxLength = 10;
    openWithMaxLength = 10;

    initializeActionLibrary();

    recentsSaveTimer = new QTimer(this);
    recentsSaveTimer->setSingleShot(true);
    recentsSaveTimer->setInterval(500);
    connect(recentsSaveTimer, &QTimer::timeout, this, &ActionManager::saveRecentsList);

    loadRecentsList();

#ifdef Q_OS_MACOS
    windowMenu = new QMenu(tr("Window"));
    QVCocoaFunctions::setWindowMenu(windowMenu);
#endif

    // Connect to settings signal
    connect(&qvApp->getSettingsManager(), &SettingsManager::settingsUpdated, this, &ActionManager::settingsUpdated);
}

void ActionManager::settingsUpdated()
{

    auto const recentsMenus = menuCloneLibrary.values("recents");
    for (const auto &recentsMenu : recentsMenus)
    {
        recentsMenu->menuAction()->setVisible(isSaveRecentsEnabled);
    }
    if (!isSaveRecentsEnabled)
        clearRecentsList();
}

QAction *ActionManager::cloneAction(const QString &key)
{
    if (auto action = getAction(key))
    {
        auto newAction = new QAction();
        newAction->setIcon(action->icon());
        newAction->setData(action->data());
        newAction->setText(action->text());
        newAction->setMenuRole(action->menuRole());
        newAction->setEnabled(action->isEnabled());
        newAction->setShortcuts(action->shortcuts());
        actionCloneLibrary.insert(key, newAction);
        return newAction;
    }

    return nullptr;
}

QAction *ActionManager::getAction(const QString &key) const
{
    if (auto action = actionLibrary.value(key))
        return action;

    return nullptr;
}

QList<QAction*> ActionManager::getAllInstancesOfAction(const QString &key) const
{
    QList<QAction*> listOfActions = getAllClonesOfAction(key);

    if (auto mainAction = getAction(key))
            listOfActions.append(mainAction);

    return listOfActions;
}

QList<QAction*> ActionManager::getAllClonesOfAction(const QString &key) const
{
    return actionCloneLibrary.values(key);
}

QList<QAction*> ActionManager::getAllClonesOfAction(const QString &key, QWidget *parent) const
{
    QList<QAction*> listOfDistantChildActions;
    const auto &actions = getAllClonesOfAction(key);
    for (const auto &action : actions)
    {
        if (action->associatedWidgets().isEmpty())
            continue;

        auto *parentWidget = action->associatedWidgets().first()->parentWidget();

        if (parentWidget == parent || (parentWidget && parentWidget->parent() == parent))
        {
            listOfDistantChildActions.append(action);
        }
    };
    return listOfDistantChildActions;
}

void ActionManager::untrackClonedActions(const QList<QAction*> &actions)
{
    for (const auto &action : actions)
    {
        QString key = action->data().toString();
        if (auto menu = action->menu())
        {
            menuCloneLibrary.remove(key, menu);
        }
        else
        {
            actionCloneLibrary.remove(key, action);
        }
    }
}

void ActionManager::untrackClonedActions(const QMenu *menu)
{
    untrackClonedActions(getAllNestedActions(menu->actions()));
}

void ActionManager::untrackClonedActions(const QMenuBar *menuBar)
{
    untrackClonedActions(getAllNestedActions(menuBar->actions()));
}

QMenuBar *ActionManager::buildMenuBar(QWidget *parent)
{
    // Global menubar
    auto *menuBar = new QMenuBar(parent);

    // Beginning of file menu
    auto *fileMenu = new QMenu(tr("File"), menuBar);

#ifdef Q_OS_MACOS
    fileMenu->addAction(cloneAction("newwindow"));
#endif
    fileMenu->addAction(cloneAction("open"));
    fileMenu->addAction(cloneAction("openurl"));
    fileMenu->addMenu(buildRecentsMenu(true, menuBar));
#ifdef Q_OS_MACOS
    fileMenu->addSeparator();
    fileMenu->addAction(cloneAction("closewindow"));
    fileMenu->addAction(cloneAction("closeallwindows"));
    QVCocoaFunctions::setAlternates(fileMenu, fileMenu->actions().length()-1, fileMenu->actions().length()-2);
#endif
    fileMenu->addSeparator();
    fileMenu->addMenu(buildOpenWithMenu(menuBar));
    fileMenu->addAction(cloneAction("opencontainingfolder"));
    fileMenu->addAction(cloneAction("showfileinfo"));
    fileMenu->addSeparator();
    fileMenu->addAction(cloneAction("quit"));

    menuBar->addMenu(fileMenu);
    // End of file menu

    // Beginning of edit menu
    auto *editMenu = new QMenu(tr("Edit"), menuBar);

    editMenu->addAction(cloneAction("copy"));
    editMenu->addAction(cloneAction("paste"));
    editMenu->addAction(cloneAction("rename"));

    menuBar->addMenu(editMenu);
    // End of edit menu

    // Beginning of view menu
    menuBar->addMenu(buildViewMenu(false, menuBar));
    // End of view menu

    // Beginning of go menu
    auto *goMenu = new QMenu(tr("Go"), menuBar);

    goMenu->addAction(cloneAction("firstfile"));
    goMenu->addAction(cloneAction("previousfile"));
    goMenu->addAction(cloneAction("nextfile"));
    goMenu->addAction(cloneAction("lastfile"));

    menuBar->addMenu(goMenu);
    // End of go menu

    // Beginning of tools menu
    menuBar->addMenu(buildToolsMenu(false, menuBar));
    // End of tools menu

    // Beginning of window menu
#ifdef Q_OS_MACOS
    menuBar->addMenu(windowMenu);
#endif
    // End of window menu

    // Beginning of help menu
    menuBar->addMenu(buildHelpMenu(false, menuBar));
    // End of help menu

    return menuBar;
}

QMenu *ActionManager::buildViewMenu(bool addIcon, QWidget *parent)
{
    auto *viewMenu = new QMenu(tr("View"), parent);
    viewMenu->menuAction()->setData("view");
    if (addIcon)
        viewMenu->setIcon(QIcon::fromTheme("zoom-fit-best"));

    viewMenu->addAction(cloneAction("zoomin"));
    viewMenu->addAction(cloneAction("zoomout"));
    viewMenu->addAction(cloneAction("resetzoom"));
    viewMenu->addAction(cloneAction("originalsize"));
    viewMenu->addSeparator();
    viewMenu->addAction(cloneAction("rotateright"));
    viewMenu->addAction(cloneAction("rotateleft"));
    viewMenu->addSeparator();
    viewMenu->addAction(cloneAction("mirror"));
    viewMenu->addAction(cloneAction("flip"));
    viewMenu->addSeparator();
    viewMenu->addAction(cloneAction("fullscreen"));

    menuCloneLibrary.insert(viewMenu->menuAction()->data().toString(), viewMenu);
    return viewMenu;
}

QMenu *ActionManager::buildToolsMenu(bool addIcon, QWidget *parent)
{
    auto *toolsMenu = new QMenu(tr("Tools"), parent);
    toolsMenu->menuAction()->setData("tools");
    if (addIcon)
        toolsMenu->setIcon(QIcon::fromTheme("configure", QIcon::fromTheme("preferences-other")));

    toolsMenu->addAction(cloneAction("saveframeas"));
    toolsMenu->addAction(cloneAction("pause"));
    toolsMenu->addAction(cloneAction("nextframe"));
    toolsMenu->addSeparator();
    toolsMenu->addAction(cloneAction("decreasespeed"));
    toolsMenu->addAction(cloneAction("resetspeed"));
    toolsMenu->addAction(cloneAction("increasespeed"));
    toolsMenu->addSeparator();
    toolsMenu->addAction(cloneAction("slideshow"));
    toolsMenu->addAction(cloneAction("options"));

    menuCloneLibrary.insert(toolsMenu->menuAction()->data().toString(), toolsMenu);
    return toolsMenu;
}

QMenu *ActionManager::buildHelpMenu(bool addIcon, QWidget *parent)
{
    auto *helpMenu = new QMenu(tr("Help"), parent);
    helpMenu->menuAction()->setData("help");
    if (addIcon)
        helpMenu->setIcon(QIcon::fromTheme("help-about"));

    helpMenu->addAction(cloneAction("about"));
    helpMenu->addAction(cloneAction("welcome"));

    menuCloneLibrary.insert(helpMenu->menuAction()->data().toString(), helpMenu);
    return helpMenu;
}

QMenu *ActionManager::buildRecentsMenu(bool includeClearAction, QWidget *parent)
{
    auto *recentsMenu = new QMenu(tr("Open Recent"), parent);
    recentsMenu->menuAction()->setData("recents");
    recentsMenu->setIcon(QIcon::fromTheme("document-open-recent"));

    connect(recentsMenu, &QMenu::aboutToShow, [this]{
        this->loadRecentsList();
    });

    for ( int i = 0; i < recentsListMaxLength; i++ )
    {
        auto action = new QAction(tr("Empty"), this);
        action->setVisible(false);
        action->setIconVisibleInMenu(true);
        action->setData("recent" + QString::number(i));

        recentsMenu->addAction(action);
        actionCloneLibrary.insert(action->data().toStringList().first(), action);
    }

    if (includeClearAction)
    {
        recentsMenu->addSeparator();
        recentsMenu->addAction(cloneAction("clearrecents"));
    }

    menuCloneLibrary.insert(recentsMenu->menuAction()->data().toString(), recentsMenu);
    updateRecentsMenu();
    // update settings whenever recent menu is created so it can possibly be hidden
    settingsUpdated();
    return recentsMenu;
}

void ActionManager::loadRecentsList()
{
    // Prevents weird bugs when opening the recent menu while the save timer is still running
    if (recentsSaveTimer->isActive())
        return;

    QSettings settings;
    settings.beginGroup("recents");

    QVariantList variantListRecents = settings.value("recentFiles").toList();
    recentsList = variantListToRecentsList(variantListRecents);

    auditRecentsList();
}

void ActionManager::saveRecentsList()
{
    auditRecentsList();

    QSettings settings;
    settings.beginGroup("recents");

    auto variantList = recentsListToVariantList(recentsList);

    settings.setValue("recentFiles", variantList);
}


void ActionManager::addFileToRecentsList(const QFileInfo &file)
{
    recentsList.prepend({file.fileName(), file.filePath()});
    auditRecentsList();
    recentsSaveTimer->start();
}

void ActionManager::auditRecentsList()
{
    // This function should be called whenever there is a change to recentsList,
    // and take care not to call any functions that call it.
    if (!isSaveRecentsEnabled)
    {
        recentsList.clear();
    }

    for (int i = 0; i < recentsList.length(); i++)
    {
        // Ensure each file exists
        auto recent =  recentsList.value(i);
        if (!QFileInfo::exists(recent.filePath))
        {
            recentsList.removeAt(i);
        }

        // Check for duplicates
        for (int i2 = 0; i2 < recentsList.length(); i2++)
        {
            if (i == i2)
                continue;

            if (recent == recentsList.value(i2))
            {
                recentsList.removeAt(i2);
            }
        }
    }

    while (recentsList.size() > recentsListMaxLength)
    {
        recentsList.removeLast();
    }

    updateRecentsMenu();
}

void ActionManager::clearRecentsList()
{
    recentsList.clear();
    saveRecentsList();
}

void ActionManager::updateRecentsMenu()
{
    for (int i = 0; i < recentsListMaxLength; i++)
    {
        const auto values = actionCloneLibrary.values("recent" + QString::number(i));
        for (const auto &action : values)
        {
            // If we are within the bounds of the recent list
            if (i < recentsList.length())
            {
                auto recent = recentsList.value(i);

                action->setVisible(true);
                action->setText(recent.fileName);

#if defined Q_OS_UNIX & !defined Q_OS_MACOS
                // set icons for linux users
                QMimeDatabase mimedb;
                QMimeType type = mimedb.mimeTypeForFile(recent.filePath);
                action->setIcon(QIcon::fromTheme(type.iconName(), QIcon::fromTheme(type.genericIconName())));
#else
                // set icons for mac/windows users
                QFileIconProvider provider;
                action->setIcon(provider.icon(QFileInfo(recent.filePath)));
#endif
            }
            else
            {
                action->setVisible(false);
                action->setText(tr("Empty"));
            }
        }
    }
    emit recentsMenuUpdated();
}

QMenu *ActionManager::buildOpenWithMenu(QWidget *parent)
{
    auto *openWithMenu = new QMenu(tr("Open With"), parent);
    openWithMenu->menuAction()->setData("openwith");
    openWithMenu->setIcon(QIcon::fromTheme("system-run"));

    for (int i = 0; i < openWithMaxLength; i++)
    {
        auto action = new QAction(tr("Empty"), this);
        action->setVisible(false);
        action->setIconVisibleInMenu(true);
        action->setData(QStringList({"openwith" + QString::number(i), ""}));

        openWithMenu->addAction(action);
        actionCloneLibrary.insert(action->data().toStringList().first(), action);
    }

    openWithMenu->addSeparator();
    openWithMenu->addAction(cloneAction("openwithother"));

    menuCloneLibrary.insert(openWithMenu->menuAction()->data().toString(), openWithMenu);
    // update settings whenever recent menu is created so it can possibly be hidden
    settingsUpdated();
    return openWithMenu;
}

void ActionManager::actionTriggered(QAction *triggeredAction)
{
    auto key = triggeredAction->data().toStringList().first();

    // For some actions, do not look for a relevant window
    if (key == "newwindow" || key == "quit" || key == "clearrecents" ||  key == "open")
    {
        actionTriggered(triggeredAction, nullptr);
        return;
    }

    // If some actions are triggered without an explicit window, we want
    // to give them a window without an image open
    bool shouldBeEmpty = false;
    if (key.startsWith("recent") || key == "openurl")
        shouldBeEmpty = true;

    if (auto *window = qvApp->getMainWindow(shouldBeEmpty))
        actionTriggered(triggeredAction, window);
}

void ActionManager::actionTriggered(QAction *triggeredAction, MainWindow *relevantWindow)
{
    auto key = triggeredAction->data().toStringList().first();
    if (key.startsWith("recent"))
    {
        QChar finalChar = key.at(key.length()-1);
        relevantWindow->openRecent(finalChar.digitValue());
    }
    else if (key.startsWith("openwith"))
    {
        const QString &exec = triggeredAction->data().toStringList().value(1);
        relevantWindow->openWith(exec);
    }

    if (key == "quit") {
        if (relevantWindow) // if a window was passed
            relevantWindow->close(); // close it so geometry is saved
        QCoreApplication::quit();
    } else if (key == "newwindow") {
        qvApp->newWindow();
    } else if (key == "open") {
        qvApp->pickFile(relevantWindow);
    } else if (key == "openurl") {
        relevantWindow->pickUrl();
    } else if (key == "closewindow") {
        auto *active = QApplication::activeWindow();
#ifdef Q_OS_MACOS
        QVCocoaFunctions::closeWindow(active->windowHandle());
#endif
        active->close();
    } else if (key == "closeallwindows") {
        for (auto *widget : QApplication::topLevelWidgets()) {
#ifdef Q_OS_MACOS
            QVCocoaFunctions::closeWindow(widget->windowHandle());
#endif
            widget->close();
        }
    } else if (key == "opencontainingfolder") {
        relevantWindow->openContainingFolder();
    } else if (key == "showfileinfo") {
        relevantWindow->showFileInfo();
    } else if (key == "copy") {
        relevantWindow->copy();
    } else if (key == "paste") {
        relevantWindow->paste();
    } else if (key == "rename") {
        relevantWindow->rename();
    } else if (key == "zoomin") {
        relevantWindow->zoomIn();
    } else if (key == "zoomout") {
        relevantWindow->zoomOut();
    } else if (key == "resetzoom") {
        relevantWindow->resetZoom();
    } else if (key == "originalsize") {
        relevantWindow->originalSize();
    } else if (key == "rotateright") {
        relevantWindow->rotateRight();
    } else if (key == "rotateleft") {
        relevantWindow->rotateLeft();
    } else if (key == "mirror") {
        relevantWindow->mirror();
    } else if (key == "flip") {
        relevantWindow->flip();
    } else if (key == "fullscreen") {
        relevantWindow->toggleFullScreen();
    } else if (key == "firstfile") {
        relevantWindow->firstFile();
    } else if (key == "previousfile") {
        relevantWindow->previousFile();
    } else if (key == "nextfile") {
        relevantWindow->nextFile();
    } else if (key == "lastfile") {
        relevantWindow->lastFile();
    } else if (key == "saveframeas") {
        relevantWindow->saveFrameAs();
    } else if (key == "pause") {
        relevantWindow->pause();
    } else if (key == "nextframe") {
        relevantWindow->nextFrame();
    } else if (key == "decreasespeed") {
        relevantWindow->decreaseSpeed();
    } else if (key == "resetspeed") {
        relevantWindow->resetSpeed();
    } else if (key == "increasespeed") {
        relevantWindow->increaseSpeed();
    } else if (key == "slideshow") {
        relevantWindow->toggleSlideshow();
    } else if (key == "options") {
        qvApp->openOptionsDialog(relevantWindow);
    } else if (key == "about") {
        qvApp->openAboutDialog();
    } else if (key == "welcome") {
        qvApp->openWelcomeDialog();
    } else if (key == "clearrecents") {
        qvApp->getActionManager().clearRecentsList();
    }
}

void ActionManager::initializeActionLibrary()
{
    auto *quitAction = new QAction(QIcon::fromTheme("application-exit"), tr("Quit"));
    actionLibrary.insert("quit", quitAction);
#ifdef Q_OS_WIN
    quitAction->setText(tr("Exit"));
#endif

    auto *newWindowAction = new QAction(QIcon::fromTheme("window-new"), tr("New Window"));
    actionLibrary.insert("newwindow", newWindowAction);

    auto *openAction = new QAction(QIcon::fromTheme("document-open"), tr("Open..."));
    actionLibrary.insert("open", openAction);

    auto *openUrlAction = new QAction(QIcon::fromTheme("document-open-remote", QIcon::fromTheme("folder-remote")), tr("Open URL..."));
    actionLibrary.insert("openurl", openUrlAction);

    auto *closeWindowAction = new QAction(QIcon::fromTheme("window-close"), tr("Close Window"));
    actionLibrary.insert("closewindow", closeWindowAction);

    auto *closeAllWindowsAction = new QAction(QIcon::fromTheme("window-close"), tr("Close All"));
    actionLibrary.insert("closeallwindows", closeAllWindowsAction);

    auto *openContainingFolderAction = new QAction(QIcon::fromTheme("document-open"), tr("Open Containing Folder"));
#ifdef Q_OS_WIN
    openContainingFolderAction->setText(tr("Show in Explorer"));
#elif defined Q_OS_MACOS
    openContainingFolderAction->setText(tr("Show in Finder"));
#endif
    openContainingFolderAction->setData({"disable"});
    actionLibrary.insert("opencontainingfolder", openContainingFolderAction);

    auto *showFileInfoAction = new QAction(QIcon::fromTheme("document-properties"), tr("Show File Info"));
    showFileInfoAction->setData({"disable"});
    actionLibrary.insert("showfileinfo", showFileInfoAction);

    auto *copyAction = new QAction(QIcon::fromTheme("edit-copy"), tr("Copy"));
    copyAction->setData({"disable"});
    actionLibrary.insert("copy", copyAction);

    auto *pasteAction = new QAction(QIcon::fromTheme("edit-paste"), tr("Paste"));
    actionLibrary.insert("paste", pasteAction);

    auto *renameAction = new QAction(QIcon::fromTheme("edit-rename", QIcon::fromTheme("document-properties")) , tr("Rename..."));
    renameAction->setData({"disable"});
    actionLibrary.insert("rename", renameAction);

    auto *zoomInAction = new QAction(QIcon::fromTheme("zoom-in"), tr("Zoom In"));
    zoomInAction->setData({"disable"});
    actionLibrary.insert("zoomin", zoomInAction);

    auto *zoomOutAction = new QAction(QIcon::fromTheme("zoom-out"), tr("Zoom Out"));
    zoomOutAction->setData({"disable"});
    actionLibrary.insert("zoomout", zoomOutAction);

    auto *resetZoomAction = new QAction(QIcon::fromTheme("zoom-fit-best"), tr("Reset Zoom"));
    resetZoomAction->setData({"disable"});
    actionLibrary.insert("resetzoom", resetZoomAction);

    auto *originalSizeAction = new QAction(QIcon::fromTheme("zoom-original"), tr("Original Size"));
    originalSizeAction->setData({"disable"});
    actionLibrary.insert("originalsize", originalSizeAction);

    auto *rotateRightAction = new QAction(QIcon::fromTheme("object-rotate-right"), tr("Rotate Right"));
    rotateRightAction->setData({"disable"});
    actionLibrary.insert("rotateright", rotateRightAction);

    auto *rotateLeftAction = new QAction(QIcon::fromTheme("object-rotate-left"), tr("Rotate Left"));
    rotateLeftAction->setData({"disable"});
    actionLibrary.insert("rotateleft", rotateLeftAction);

    auto *mirrorAction = new QAction(QIcon::fromTheme("object-flip-horizontal"), tr("Mirror"));
    mirrorAction->setData({"disable"});
    actionLibrary.insert("mirror", mirrorAction);

    auto *flipAction = new QAction(QIcon::fromTheme("object-flip-vertical"), tr("Flip"));
    flipAction->setData({"disable"});
    actionLibrary.insert("flip", flipAction);

    auto *fullScreenAction = new QAction(QIcon::fromTheme("view-fullscreen"), tr("Enter Full Screen"));
    fullScreenAction->setMenuRole(QAction::NoRole);
    actionLibrary.insert("fullscreen", fullScreenAction);

    auto *firstFileAction = new QAction(QIcon::fromTheme("go-first"), tr("First File"));
    firstFileAction->setData({"disable"});
    actionLibrary.insert("firstfile", firstFileAction);

    auto *previousFileAction = new QAction(QIcon::fromTheme("go-previous"), tr("Previous File"));
    previousFileAction->setData({"disable"});
    actionLibrary.insert("previousfile", previousFileAction);

    auto *nextFileAction = new QAction(QIcon::fromTheme("go-next"), tr("Next File"));
    nextFileAction->setData({"disable"});
    actionLibrary.insert("nextfile", nextFileAction);

    auto *lastFileAction = new QAction(QIcon::fromTheme("go-last"), tr("Last File"));
    lastFileAction->setData({"disable"});
    actionLibrary.insert("lastfile", lastFileAction);

    auto *saveFrameAsAction = new QAction(QIcon::fromTheme("document-save-as"), tr("Save Frame As..."));
    saveFrameAsAction->setData({"gifdisable"});
    actionLibrary.insert("saveframeas", saveFrameAsAction);

    auto *pauseAction = new QAction(QIcon::fromTheme("media-playback-pause"), tr("Pause"));
    pauseAction->setData({"gifdisable"});
    actionLibrary.insert("pause", pauseAction);

    auto *nextFrameAction = new QAction(QIcon::fromTheme("media-skip-forward"), tr("Next Frame"));
    nextFrameAction->setData({"gifdisable"});
    actionLibrary.insert("nextframe", nextFrameAction);

    auto *decreaseSpeedAction = new QAction(QIcon::fromTheme("media-seek-backward"), tr("Decrease Speed"));
    decreaseSpeedAction->setData({"gifdisable"});
    actionLibrary.insert("decreasespeed", decreaseSpeedAction);

    auto *resetSpeedAction = new QAction(QIcon::fromTheme("media-playback-start"), tr("Reset Speed"));
    resetSpeedAction->setData({"gifdisable"});
    actionLibrary.insert("resetspeed", resetSpeedAction);

    auto *increaseSpeedAction = new QAction(QIcon::fromTheme("media-skip-forward"), tr("Increase Speed"));
    increaseSpeedAction->setData({"gifdisable"});
    actionLibrary.insert("increasespeed", increaseSpeedAction);

    auto *slideshowAction = new QAction(QIcon::fromTheme("media-playback-start"), tr("Start Slideshow"));
    slideshowAction->setData({"disable"});
    actionLibrary.insert("slideshow", slideshowAction);

    auto *optionsAction = new QAction(QIcon::fromTheme("configure", QIcon::fromTheme("preferences-other")), tr("Options"));
#if defined Q_OS_UNIX & !defined Q_OS_MACOS
    optionsAction->setText(tr("Preferences"));
#elif defined Q_OS_MACOS
    optionsAction->setText(tr("Preferences..."));
#endif
    actionLibrary.insert("options", optionsAction);

    auto *aboutAction = new QAction(QIcon::fromTheme("help-about"), tr("About"));
#ifdef Q_OS_MACOS
    aboutAction->setText(tr("About qView"));
#endif
    actionLibrary.insert("about", aboutAction);

    auto *welcomeAction = new QAction(QIcon::fromTheme("help-faq", QIcon::fromTheme("help-about")), tr("Welcome"));
    actionLibrary.insert("welcome", welcomeAction);

    // This one is kinda different so here's a separator comment
    auto *clearRecentsAction = new QAction(QIcon::fromTheme("edit-delete"), tr("Clear Menu"));
    actionLibrary.insert("clearrecents", clearRecentsAction);

    // This one is also kinda different
    auto *openWithOtherAction = new QAction(tr("Other Application..."));
    actionLibrary.insert("openwithother", openWithOtherAction);

    // Set data values and disable actions
    const auto keys = actionLibrary.keys();
    for (const auto &key : keys)
    {
        auto *value = actionLibrary.value(key);
        auto data = value->data().toStringList();
        data.prepend(key);
        value->setData(data);

        if (data.last().contains("disable"))
            value->setEnabled(false);
    }
}
