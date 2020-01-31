#include "actionmanager.h"
#include "qvapplication.h"

ActionManager::ActionManager(QObject *parent) : QObject(parent)
{
    initializeActionLibrary();
}

QAction *ActionManager::getAction(QString key)
{
    auto action = actionLibrary.value(key);

    if (action)
        return action;

    return nullptr;
}

QMenuBar *ActionManager::buildMenuBar()
{
    //Global menubar
    auto *menuBar = new QMenuBar();

    //Beginning of file menu
    auto *fileMenu = new QMenu(tr("File"));

    fileMenu->addAction(getAction("newwindow"));
    fileMenu->addAction(getAction("open"));
    fileMenu->addAction(getAction("openurl"));
    fileMenu->addAction(getAction("closewindow"));
    fileMenu->addSeparator();
    fileMenu->addAction(getAction("opencontainingfolder"));
    fileMenu->addAction(getAction("showfileinfo"));

    menuBar->addMenu(fileMenu);
    //End of file menu

    //Beginning of edit menu
    auto *editMenu = new QMenu(tr("Edit"));

    editMenu->addAction(getAction("copy"));
    editMenu->addAction(getAction("paste"));

    menuBar->addMenu(editMenu);
    //End of edit menu

    //Beginning of view menu
    auto *viewMenu = new QMenu(tr("View"));

    viewMenu->addAction(getAction("zoomin"));
    viewMenu->addAction(getAction("zoomout"));
    viewMenu->addAction(getAction("resetzoom"));
    viewMenu->addAction(getAction("originalsize"));
    viewMenu->addSeparator();
    viewMenu->addAction(getAction("rotateright"));
    viewMenu->addAction(getAction("rotateleft"));
    viewMenu->addSeparator();
    viewMenu->addAction(getAction("mirror"));
    viewMenu->addAction(getAction("flip"));

    menuBar->addMenu(viewMenu);
    //End of view menu

    //Beginning of go menu
    auto *goMenu = new QMenu(tr("Go"));

    goMenu->addAction(getAction("firstfile"));
    goMenu->addAction(getAction("previousfile"));
    goMenu->addAction(getAction("nextfile"));
    goMenu->addAction(getAction("lastfile"));

    menuBar->addMenu(goMenu);
    //End of go menu

    //Beginning of tools menu
    auto *toolsMenu = new QMenu(tr("Tools"));

    toolsMenu->addMenu(buildGifMenu());
    toolsMenu->addAction(getAction("slideshow"));
    toolsMenu->addAction(getAction("options"));

    menuBar->addMenu(toolsMenu);
    //End of tools menu

    //Beginning of help menu
    auto *helpMenu = new QMenu(tr("Help"));

    helpMenu->addAction(getAction("about"));
    helpMenu->addAction(getAction("welcome"));

    menuBar->addMenu(helpMenu);
    //End of help menu

    return menuBar;
}

QMenu *ActionManager::buildGifMenu()
{
    auto *gifMenu = new QMenu(tr("GIF Controls"));

    gifMenu->addAction(getAction("saveframeas"));
    gifMenu->addAction(getAction("pause"));
    gifMenu->addAction(getAction("nextframe"));
    gifMenu->addSeparator();
    gifMenu->addAction(getAction("decreasespeed"));
    gifMenu->addAction(getAction("resetspeed"));
    gifMenu->addAction(getAction("increasespeed"));

    return gifMenu;
}

void ActionManager::initializeActionLibrary()
{
    auto *newWindowAction = new QAction(QIcon::fromTheme("window-new"), tr("New Window"));
    connect(newWindowAction, &QAction::triggered, [](){
        qvApp->newWindow();
    });
    actionLibrary.insert("newwindow", newWindowAction);

    auto *openAction = new QAction(QIcon::fromTheme("document-open"), tr("Open..."));
    connect(openAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->pickFile();
    });
    actionLibrary.insert("open", openAction);

    auto *openUrlAction = new QAction(QIcon::fromTheme("document-open-remote", QIcon::fromTheme("folder-remote")), tr("Open URL..."));
    connect(openUrlAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->pickUrl();
    });
    actionLibrary.insert("openurl", openUrlAction);

    auto *closeWindowAction = new QAction(QIcon::fromTheme("window-close"), tr("Close Window"));
    connect(closeWindowAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->close();
    });
    actionLibrary.insert("closewindow", closeWindowAction);

    auto *openContainingFolderAction = new QAction(QIcon::fromTheme("document-open"), tr("Open Containing Folder..."));
    #if defined Q_OS_WIN
        actionOpenContainingFolder->setText(tr("Show in Explorer"));
    #elif defined Q_OS_MACX
        openContainingFolderAction->setText(tr("Show in Finder"));
    #endif
    connect(openContainingFolderAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->openContainingFolder();
    });
    actionLibrary.insert("opencontainingfolder", openContainingFolderAction);

    auto *showFileInfoAction = new QAction(QIcon::fromTheme("document-properties"), tr("Show File Info"));
    connect(showFileInfoAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->showFileInfo();
    });
    actionLibrary.insert("showfileinfo", showFileInfoAction);

    auto *copyAction = new QAction(QIcon::fromTheme("edit-copy"), tr("Copy"));
    connect(copyAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->copy();
    });
    actionLibrary.insert("copy", copyAction);

    auto *pasteAction = new QAction(QIcon::fromTheme("edit-paste"), tr("Paste"));
    connect(pasteAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->paste();
    });
    actionLibrary.insert("paste", pasteAction);

    auto *zoomInAction = new QAction(QIcon::fromTheme("zoom-in"), tr("Zoom In"));
    connect(zoomInAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->zoomIn();
    });
    actionLibrary.insert("zoomin", zoomInAction);

    auto *zoomOutAction = new QAction(QIcon::fromTheme("zoom-out"), tr("Zoom Out"));
    connect(zoomOutAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->zoomOut();
    });
    actionLibrary.insert("zoomout", zoomOutAction);

    auto *resetZoomAction = new QAction(QIcon::fromTheme("zoom-fit-best"), tr("Reset Zoom"));
    connect(resetZoomAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->resetZoom();
    });
    actionLibrary.insert("resetzoom", resetZoomAction);

    auto *originalSizeAction = new QAction(QIcon::fromTheme("zoom-original"), tr("Original Size"));
    connect(originalSizeAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->originalSize();
    });
    actionLibrary.insert("originalsize", originalSizeAction);

    auto *rotateRightAction = new QAction(QIcon::fromTheme("object-rotate-right"), tr("Rotate Right"));
    connect(rotateRightAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->rotateRight();
    });
    actionLibrary.insert("rotateright", rotateRightAction);

    auto *rotateLeftAction = new QAction(QIcon::fromTheme("object-rotate-left"), tr("Rotate Left"));
    connect(rotateLeftAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->rotateLeft();
    });
    actionLibrary.insert("rotateleft", rotateLeftAction);

    auto *mirrorAction = new QAction(QIcon::fromTheme("object-flip-horizontal"), tr("Mirror"));
    connect(mirrorAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->mirror();
    });
    actionLibrary.insert("mirror", mirrorAction);

    auto *flipAction = new QAction(QIcon::fromTheme("object-flip-vertical"), tr("Flip"));
    connect(flipAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->flip();
    });
    actionLibrary.insert("flip", flipAction);

    auto *firstFileAction = new QAction(QIcon::fromTheme("go-first"), tr("First File"));
    connect(firstFileAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->firstFile();
    });
    actionLibrary.insert("firstfile", firstFileAction);

    auto *previousFileAction = new QAction(QIcon::fromTheme("go-previous"), tr("Previous File"));
    connect(previousFileAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->previousFile();
    });
    actionLibrary.insert("previousfile", previousFileAction);

    auto *nextFileAction = new QAction(QIcon::fromTheme("go-next"), tr("Next File"));
    connect(nextFileAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->nextFile();
    });
    actionLibrary.insert("nextfile", nextFileAction);

    auto *lastFileAction = new QAction(QIcon::fromTheme("go-last"), tr("Last File"));
    connect(lastFileAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->lastFile();
    });
    actionLibrary.insert("lastfile", lastFileAction);

    auto *saveFrameAsAction = new QAction(QIcon::fromTheme("document-save-as"), tr("Save Frame As..."));
    actionLibrary.insert("saveframeas", saveFrameAsAction);

    auto *pauseAction = new QAction(QIcon::fromTheme("media-playback-pause"), tr("Pause"));
    actionLibrary.insert("pause", pauseAction);

    auto *nextFrameAction = new QAction(QIcon::fromTheme("media-skip-forward"), tr("Next Frame"));
    actionLibrary.insert("nextframe", nextFrameAction);

    auto *decreaseSpeedAction = new QAction(QIcon::fromTheme("media-seek-backward"), tr("Decrease Speed"));
    actionLibrary.insert("decreasespeed", decreaseSpeedAction);

    auto *resetSpeedAction = new QAction(QIcon::fromTheme("media-playback-start"), tr("Reset Speed"));
    actionLibrary.insert("resetspeed", resetSpeedAction);

    auto *increaseSpeedAction = new QAction(QIcon::fromTheme("media-skip-forward"), tr("Increase Speed"));
    actionLibrary.insert("increasespeed", increaseSpeedAction);

    auto *slideshowAction = new QAction(QIcon::fromTheme("media-playback-start"), tr("Start Slideshow"));
    actionLibrary.insert("slideshow", slideshowAction);

    auto *optionsAction = new QAction(QIcon::fromTheme("configure", QIcon::fromTheme("preferences-other")), tr("Options"));
    #if defined Q_OS_UNIX & !defined Q_OS_MACOS
        optionsAction->setText(tr("Preferences"));
    #elif defined Q_OS_MACOS
        optionsAction->setText(tr("Preferences..."));
    #endif
    connect(optionsAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->openOptions();
    });
    actionLibrary.insert("options", optionsAction);

    auto *aboutAction = new QAction(QIcon::fromTheme("help-about"), tr("About"));
    #if defined Q_OS_MACOS
        aboutAction->setText("About qView");
    #endif
    connect(aboutAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->openAbout();
    });
    actionLibrary.insert("about", aboutAction);

    auto *welcomeAction = new QAction(QIcon::fromTheme("help-faq", QIcon::fromTheme("help-about")), tr("Welcome"));
    connect(welcomeAction, &QAction::triggered, [](){
        if (auto *window = qvApp->getCurrentMainWindow())
            window->openWelcome();
    });
    actionLibrary.insert("welcome", welcomeAction);
}
