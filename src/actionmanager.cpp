#include "actionmanager.h"
#include "qvapplication.h"

#include <QSettings>
#include <QMimeDatabase>
#include <QTimer>

#include <QDebug>

ActionManager::ActionManager(QObject *parent) : QObject(parent)
{
    isSaveRecentsEnabled = true;

    recentsListMaxLength = 10;

    initializeActionLibrary();

    initializeShortcutsList();
    updateShortcuts();

    loadRecentsList();

    recentsSaveTimer = new QTimer(this);
    recentsSaveTimer->setSingleShot(true);
    recentsSaveTimer->setInterval(500);
    connect(recentsSaveTimer, &QTimer::timeout, this, &ActionManager::saveRecentsList);
}

QAction *ActionManager::cloneAction(const QString &key)
{
    if (auto action = getAction(key))
    {
        auto newAction = new QAction();
        newAction->setIcon(action->icon());
        newAction->setData(action->data());
        newAction->setText(action->text());
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

QList<QAction*> ActionManager::getCloneActions(const QString &key) const
{
    return actionCloneLibrary.values(key);
}

QList<QAction*> ActionManager::getAllInstancesOfAction(const QString &key) const
{
    QList<QAction*> listOfActions;

    if (auto mainAction = getAction(key))
        listOfActions.append(mainAction);

    listOfActions.append(getCloneActions(key));

    return listOfActions;
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
    fileMenu->addAction(cloneAction("closewindow"));
    #endif
    fileMenu->addSeparator();
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

    menuBar->addMenu(editMenu);
    // End of edit menu

    // Beginning of view menu
    bool withFullscreen = true;

    #ifdef Q_OS_MACOS
        withFullscreen = false;
    #endif

    menuBar->addMenu(buildViewMenu(false, withFullscreen, menuBar));
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

    // Beginning of help menu
    menuBar->addMenu(buildHelpMenu(false, menuBar));
    // End of help menu

    return menuBar;
}

QMenu *ActionManager::buildGifMenu(QWidget *parent)
{
    auto *gifMenu = new QMenu(tr("GIF Controls"), parent);
    gifMenu->menuAction()->setData("gif");
    gifMenu->setIcon(QIcon::fromTheme("media-playlist-repeat"));

    gifMenu->addAction(cloneAction("saveframeas"));
    gifMenu->addAction(cloneAction("pause"));
    gifMenu->addAction(cloneAction("nextframe"));
    gifMenu->addSeparator();
    gifMenu->addAction(cloneAction("decreasespeed"));
    gifMenu->addAction(cloneAction("resetspeed"));
    gifMenu->addAction(cloneAction("increasespeed"));

    menuCloneLibrary.insert(gifMenu->menuAction()->data().toString(), gifMenu);
    return gifMenu;
}

QMenu *ActionManager::buildViewMenu(bool addIcon, bool withFullscreen, QWidget *parent)
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
    if (withFullscreen)
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

    toolsMenu->addMenu(buildGifMenu(toolsMenu));
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
        action->setData("recent" + QString::number(i));

        recentsMenu->addAction(action);
        actionCloneLibrary.insert(action->data().toString(), action);
    }

    if (includeClearAction)
    {
        recentsMenu->addSeparator();
        recentsMenu->addAction(cloneAction("clearrecents"));
    }

    menuCloneLibrary.insert(recentsMenu->menuAction()->data().toString(), recentsMenu);
    updateRecentsMenu();
    return recentsMenu;
}

void ActionManager::loadRecentsList()
{
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
            auto recent = recentsList.value(i);

            // If we are within the bounds of the recent list
            if (i < recentsList.length())
            {
                action->setVisible(true);
                action->setText(recent.fileName);

                //set icons for linux users
                QMimeDatabase mimedb;
                QMimeType type = mimedb.mimeTypeForFile(recent.filePath);
                if (!type.iconName().isNull())
                {
                    action->setIcon(QIcon::fromTheme(type.iconName()));
                }
                else
                {
                    action->setIcon(QIcon::fromTheme(type.genericIconName()));
                }

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

void ActionManager::actionTriggered(QAction *triggeredAction) const
{ 
    auto key = triggeredAction->data().toString();

    // For some actions, do not look for a relevant window
    if (key == "newwindow" || key == "quit" || key == "clearrecents")
    {
        actionTriggered(triggeredAction, nullptr);
        return;
    }

    // If some actions are triggered without an explicit window, we want
    // to give them a window without an image open
    bool shouldBeEmpty = false;
    if (key.startsWith("recent") || key == "open" || key == "openurl")
        shouldBeEmpty = true;

    if (auto *window = qvApp->getMainWindow(shouldBeEmpty))
        actionTriggered(triggeredAction, window);
}

void ActionManager::actionTriggered(QAction *triggeredAction, MainWindow *relevantWindow) const
{
    auto key = triggeredAction->data().toString();
    if (key.startsWith("recent"))
    {
        QChar finalChar = key.at(key.length()-1);
        relevantWindow->openRecent(finalChar.digitValue());
    }

    if (key == "quit") {
        QCoreApplication::quit();
    } else if (key == "newwindow") {
        qvApp->newWindow();
    } else if (key == "open") {
        relevantWindow->pickFile();
    } else if (key == "openurl") {
        relevantWindow->pickUrl();
    } else if (key == "closewindow") {
        relevantWindow->close();
    } else if (key == "opencontainingfolder") {
        relevantWindow->openContainingFolder();
    } else if (key == "showfileinfo") {
        relevantWindow->showFileInfo();
    } else if (key == "copy") {
        relevantWindow->copy();
    } else if (key == "paste") {
        relevantWindow->paste();
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
        relevantWindow->openOptions();
    } else if (key == "about") {
        relevantWindow->openAbout();
    } else if (key == "welcome") {
        relevantWindow->openWelcome();
    } else if (key == "clearrecents") {
        qvApp->getActionManager()->clearRecentsList();
    }
}

void ActionManager::initializeActionLibrary()
{
    auto *quitAction = new QAction(QIcon::fromTheme("application-exit"), tr("Quit"));
    quitAction->setData("quit");
    actionLibrary.insert("quit", quitAction);

    auto *newWindowAction = new QAction(QIcon::fromTheme("window-new"), tr("New Window"));
    newWindowAction->setData("newwindow");
    actionLibrary.insert("newwindow", newWindowAction);

    auto *openAction = new QAction(QIcon::fromTheme("document-open"), tr("Open..."));
    openAction->setData("open");
    actionLibrary.insert("open", openAction);

    auto *openUrlAction = new QAction(QIcon::fromTheme("document-open-remote", QIcon::fromTheme("folder-remote")), tr("Open URL..."));
    openUrlAction->setData("openurl");
    actionLibrary.insert("openurl", openUrlAction);

    auto *closeWindowAction = new QAction(QIcon::fromTheme("window-close"), tr("Close Window"));
    closeWindowAction->setData("closewindow");
    actionLibrary.insert("closewindow", closeWindowAction);

    auto *openContainingFolderAction = new QAction(QIcon::fromTheme("document-open"), tr("Open Containing Folder"));
    #if defined Q_OS_WIN
        openContainingFolderAction->setText(tr("Show in Explorer"));
    #elif defined Q_OS_MACOS
        openContainingFolderAction->setText(tr("Show in Finder"));
    #endif
    openContainingFolderAction->setData("opencontainingfolder");
    actionLibrary.insert("opencontainingfolder", openContainingFolderAction);

    auto *showFileInfoAction = new QAction(QIcon::fromTheme("document-properties"), tr("Show File Info"));
    showFileInfoAction->setData("showfileinfo");
    actionLibrary.insert("showfileinfo", showFileInfoAction);

    auto *copyAction = new QAction(QIcon::fromTheme("edit-copy"), tr("Copy"));
    copyAction->setData("copy");
    actionLibrary.insert("copy", copyAction);

    auto *pasteAction = new QAction(QIcon::fromTheme("edit-paste"), tr("Paste"));
    pasteAction->setData("paste");
    actionLibrary.insert("paste", pasteAction);

    auto *zoomInAction = new QAction(QIcon::fromTheme("zoom-in"), tr("Zoom In"));
    zoomInAction->setData("zoomin");
    actionLibrary.insert("zoomin", zoomInAction);

    auto *zoomOutAction = new QAction(QIcon::fromTheme("zoom-out"), tr("Zoom Out"));
    zoomOutAction->setData("zoomout");
    actionLibrary.insert("zoomout", zoomOutAction);

    auto *resetZoomAction = new QAction(QIcon::fromTheme("zoom-fit-best"), tr("Reset Zoom"));
    resetZoomAction->setData("resetzoom");
    actionLibrary.insert("resetzoom", resetZoomAction);

    auto *originalSizeAction = new QAction(QIcon::fromTheme("zoom-original"), tr("Original Size"));
    originalSizeAction->setData("originalsize");
    actionLibrary.insert("originalsize", originalSizeAction);

    auto *rotateRightAction = new QAction(QIcon::fromTheme("object-rotate-right"), tr("Rotate Right"));
    rotateRightAction->setData("rotateright");
    actionLibrary.insert("rotateright", rotateRightAction);

    auto *rotateLeftAction = new QAction(QIcon::fromTheme("object-rotate-left"), tr("Rotate Left"));
    rotateLeftAction->setData("rotateleft");
    actionLibrary.insert("rotateleft", rotateLeftAction);

    auto *mirrorAction = new QAction(QIcon::fromTheme("object-flip-horizontal"), tr("Mirror"));
    mirrorAction->setData("mirror");
    actionLibrary.insert("mirror", mirrorAction);

    auto *flipAction = new QAction(QIcon::fromTheme("object-flip-vertical"), tr("Flip"));
    flipAction->setData("flip");
    actionLibrary.insert("flip", flipAction);

    auto *fullScreenAction = new QAction(QIcon::fromTheme("view-fullscreen"), tr("Enter Full Screen"));
    fullScreenAction->setData("fullscreen");
    actionLibrary.insert("fullscreen", fullScreenAction);

    auto *firstFileAction = new QAction(QIcon::fromTheme("go-first"), tr("First File"));
    firstFileAction->setData("firstfile");
    actionLibrary.insert("firstfile", firstFileAction);

    auto *previousFileAction = new QAction(QIcon::fromTheme("go-previous"), tr("Previous File"));
    previousFileAction->setData("previousfile");
    actionLibrary.insert("previousfile", previousFileAction);

    auto *nextFileAction = new QAction(QIcon::fromTheme("go-next"), tr("Next File"));
    nextFileAction->setData("nextfile");
    actionLibrary.insert("nextfile", nextFileAction);

    auto *lastFileAction = new QAction(QIcon::fromTheme("go-last"), tr("Last File"));
    lastFileAction->setData("lastfile");
    actionLibrary.insert("lastfile", lastFileAction);

    auto *saveFrameAsAction = new QAction(QIcon::fromTheme("document-save-as"), tr("Save Frame As..."));
    saveFrameAsAction->setData("saveframeas");
    actionLibrary.insert("saveframeas", saveFrameAsAction);

    auto *pauseAction = new QAction(QIcon::fromTheme("media-playback-pause"), tr("Pause"));
    pauseAction->setData("pause");
    actionLibrary.insert("pause", pauseAction);

    auto *nextFrameAction = new QAction(QIcon::fromTheme("media-skip-forward"), tr("Next Frame"));
    nextFrameAction->setData("nextframe");
    actionLibrary.insert("nextframe", nextFrameAction);

    auto *decreaseSpeedAction = new QAction(QIcon::fromTheme("media-seek-backward"), tr("Decrease Speed"));
    decreaseSpeedAction->setData("decreasespeed");
    actionLibrary.insert("decreasespeed", decreaseSpeedAction);

    auto *resetSpeedAction = new QAction(QIcon::fromTheme("media-playback-start"), tr("Reset Speed"));
    resetSpeedAction->setData(("resetspeed"));
    actionLibrary.insert("resetspeed", resetSpeedAction);

    auto *increaseSpeedAction = new QAction(QIcon::fromTheme("media-skip-forward"), tr("Increase Speed"));
    increaseSpeedAction->setData("increasespeed");
    actionLibrary.insert("increasespeed", increaseSpeedAction);

    auto *slideshowAction = new QAction(QIcon::fromTheme("media-playback-start"), tr("Start Slideshow"));
    slideshowAction->setData("slideshow");
    actionLibrary.insert("slideshow", slideshowAction);

    auto *optionsAction = new QAction(QIcon::fromTheme("configure", QIcon::fromTheme("preferences-other")), tr("Options"));
    #if defined Q_OS_UNIX & !defined Q_OS_MACOS
        optionsAction->setText(tr("Preferences"));
    #elif defined Q_OS_MACOS
        optionsAction->setText(tr("Preferences..."));
    #endif
    optionsAction->setData("options");
    actionLibrary.insert("options", optionsAction);

    auto *aboutAction = new QAction(QIcon::fromTheme("help-about"), tr("About"));
    #if defined Q_OS_MACOS
        aboutAction->setText("About qView");
    #endif
    aboutAction->setData("about");
    actionLibrary.insert("about", aboutAction);

    auto *welcomeAction = new QAction(QIcon::fromTheme("help-faq", QIcon::fromTheme("help-about")), tr("Welcome"));
    welcomeAction->setData("welcome");
    actionLibrary.insert("welcome", welcomeAction);

    // This one is kinda different so here's a separator comment
    auto *clearRecentsAction = new QAction(QIcon::fromTheme("edit-delete"), tr("Clear Menu"));
    clearRecentsAction->setData("clearrecents");
    actionLibrary.insert("clearrecents", clearRecentsAction);
}

void ActionManager::initializeShortcutsList()
{
    shortcutsList.append({"Open", "open", keyBindingsToStringList(QKeySequence::Open), {}});
    shortcutsList.append({"Open URL", "openurl", QStringList(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O).toString()), {}});
    shortcutsList.append({tr("Open Containing Folder"), "opencontainingfolder", {}, {}});
    //Sets open containing folder action name to platform-appropriate alternative
    #ifdef Q_OS_WIN
    shortcutsList.last().readableName = tr("Show in Explorer");
    #elif defined Q_OS_MACOS
    shortcutsList.last().readableName  = tr("Show in Finder");
    #endif
    shortcutsList.append({"Show File Info", "showfileinfo", QStringList(QKeySequence(Qt::Key_I).toString()), {}});
    shortcutsList.append({"Copy", "copy", keyBindingsToStringList(QKeySequence::Copy), {}});
    shortcutsList.append({"Paste", "paste", keyBindingsToStringList(QKeySequence::Paste), {}});
    shortcutsList.append({"First File", "firstfile", QStringList(QKeySequence(Qt::Key_Home).toString()), {}});
    shortcutsList.append({"Previous File", "previousfile", QStringList(QKeySequence(Qt::Key_Left).toString()), {}});
    shortcutsList.append({"Next File", "nextfile", QStringList(QKeySequence(Qt::Key_Right).toString()), {}});
    shortcutsList.append({"Last File", "lastfile", QStringList(QKeySequence(Qt::Key_End).toString()), {}});
    shortcutsList.append({"Zoom In", "zoomin", keyBindingsToStringList(QKeySequence::ZoomIn), {}});
    // Allow zooming with Ctrl + plus like a regular person (without holding shift)
    if (!shortcutsList.last().defaultShortcuts.contains(QKeySequence(Qt::CTRL + Qt::Key_Equal).toString()))
    {
        shortcutsList.last().defaultShortcuts << QKeySequence(Qt::CTRL + Qt::Key_Equal).toString();
    }

    shortcutsList.append({"Zoom Out", "zoomout", keyBindingsToStringList(QKeySequence::ZoomOut), {}});
    shortcutsList.append({"Reset Zoom", "resetzoom", QStringList(QKeySequence(Qt::CTRL + Qt::Key_0).toString()), {}});
    shortcutsList.append({"Original Size", "originalsize", QStringList(QKeySequence(Qt::Key_O).toString()), {}});
    shortcutsList.append({"Rotate Right", "rotateright", QStringList(QKeySequence(Qt::Key_Up).toString()), {}});
    shortcutsList.append({"Rotate Left", "rotateleft", QStringList(QKeySequence(Qt::Key_Down).toString()), {}});
    shortcutsList.append({"Mirror", "mirror", QStringList(QKeySequence(Qt::Key_F).toString()), {}});
    shortcutsList.append({"Flip", "flip", QStringList(QKeySequence(Qt::CTRL + Qt::Key_F).toString()), {}});
    shortcutsList.append({"Full Screen", "fullscreen", keyBindingsToStringList(QKeySequence::FullScreen), {}});
    //Fixes alt+enter only working with numpad enter when using qt's standard keybinds
    #ifdef Q_OS_WIN
    shortcutsList.last().defaultShortcuts << QKeySequence(Qt::ALT + Qt::Key_Return).toString();
    #elif defined Q_OS_UNIX & !defined Q_OS_MACOS
    // F11 is for some reason not there by default in GNOME
    if (shortcutsList.last().defaultShortcuts.contains(QKeySequence(Qt::CTRL + Qt::Key_F11).toString()) &&
        !shortcutsList.last().defaultShortcuts.contains(QKeySequence(Qt::Key_F11).toString()))
    {
        shortcutsList.last().defaultShortcuts << QKeySequence(Qt::Key_F11).toString();
    }
    #endif
    shortcutsList.append({"Save Frame As", "saveframeas", keyBindingsToStringList(QKeySequence::Save), {}});
    shortcutsList.append({"Pause", "pause", QStringList(QKeySequence(Qt::Key_P).toString()), {}});
    shortcutsList.append({"Next Frame", "nextframe", QStringList(QKeySequence(Qt::Key_N).toString()), {}});
    shortcutsList.append({"Decrease Speed", "decreasespeed", QStringList(QKeySequence(Qt::Key_BracketLeft).toString()), {}});
    shortcutsList.append({"Reset Speed", "resetspeed", QStringList(QKeySequence(Qt::Key_Backslash).toString()), {}});
    shortcutsList.append({"Increase Speed", "increasespeed", QStringList(QKeySequence(Qt::Key_BracketRight).toString()), {}});
    shortcutsList.append({"Toggle Slideshow", "slideshow", {}, {}});
    shortcutsList.append({"Options", "options", keyBindingsToStringList(QKeySequence::Preferences), {}});
    #ifdef Q_OS_UNIX
        shortcutsList.last().readableName = tr("Preferences");
    #endif
    // mac exclusive shortcuts
    #ifdef Q_OS_MACOS
    shortcutsList.append({"New Window", "newwindow", keyBindingsToStringList(QKeySequence::New), {}});
    shortcutsList.append({"Close Window", "closewindow", QStringList(QKeySequence(Qt::CTRL + Qt::Key_W).toString()), {}});
    #endif
    shortcutsList.append({"Quit", "quit", keyBindingsToStringList(QKeySequence::Quit), {}});
}

void ActionManager::updateShortcuts()
{
    QSettings settings;
    settings.beginGroup("shortcuts");

    // Set shortcut's shortcut field to default or user-set
    QMutableListIterator<SShortcut> iter(shortcutsList);
    while (iter.hasNext())
    {
        auto value = iter.next();
        value.shortcuts = settings.value(value.name, value.defaultShortcuts).toStringList();
        iter.setValue(value);
    }

    // Set action's shortcuts to current shortcut
    for (const auto &shortcut : getShortcutsList())
    {
        const auto actionList = getAllInstancesOfAction(shortcut.name);

        for (const auto &action : actionList)
        {
            if (action)
                action->setShortcuts(stringListToKeySequenceList(shortcut.shortcuts));
        }
    }
}

void ActionManager::loadSettings()
{
    QSettings settings;
    settings.beginGroup("options");

    isSaveRecentsEnabled = settings.value("saverecents", true).toBool();
    auto const recentsMenus = menuCloneLibrary.values("recents");
    for (const auto &recentsMenu : recentsMenus)
    {
        recentsMenu->menuAction()->setVisible(isSaveRecentsEnabled);
    }
    if (!isSaveRecentsEnabled)
        clearRecentsList();
}
