#include "qvapplication.h"
#include "qvoptionsdialog.h"
#include <QFileOpenEvent>
#include <QMenu>
#include <QSettings>
#include <QMenuBar>

#include <QDebug>

QVApplication::QVApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    //don't even try to show menu icons on mac or windows
    #if defined(Q_OS_MACX) || defined(Q_OS_WIN)
    setAttribute(Qt::AA_DontShowIconsInMenus);
    #endif

    dockMenu = new QMenu();
    #ifdef Q_OS_MACX
    dockMenu->setAsDockMenu();
    setQuitOnLastWindowClosed(false);
    #endif

    //Global menubar
    auto *menuBar = new QMenuBar();

    //Beginning of file menu
    auto *fileMenu = new QMenu(tr("File"));

    auto *newWindowAction = new QAction(QIcon::fromTheme("window-new"), tr("New Window"));
    connect(newWindowAction, &QAction::triggered, [](){
        newWindow();
    });
    fileMenu->addAction(newWindowAction);

    auto *openAction = new QAction(QIcon::fromTheme("document-open"), tr("Open..."));
    connect(openAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->pickFile();
    });
    fileMenu->addAction(openAction);

    auto *openUrlAction = new QAction(QIcon::fromTheme("document-open-remote", QIcon::fromTheme("folder-remote")), tr("Open URL..."));
    connect(openUrlAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->pickUrl();
    });
    fileMenu->addAction(openUrlAction);

    auto *closeAction = new QAction(QIcon::fromTheme("window-close"), tr("Close Window"));
    connect(closeAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->close();
    });
    fileMenu->addAction(closeAction);

    fileMenu->addSeparator();

    auto *openContainingFolderAction = new QAction(QIcon::fromTheme("document-open"), tr("Open Containing Folder..."));
    #if defined Q_OS_WIN
        actionOpenContainingFolder->setText(tr("Show in Explorer"));
    #elif defined Q_OS_MACX
        openContainingFolderAction->setText(tr("Show in Finder"));
    #endif
    connect(openContainingFolderAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->openContainingFolder();
    });
    fileMenu->addAction(openContainingFolderAction);

    auto *showFileInfoAction = new QAction(QIcon::fromTheme("document-properties"), tr("Show File Info"));
    connect(showFileInfoAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->showFileInfo();
    });
    fileMenu->addAction(showFileInfoAction);

    menuBar->addMenu(fileMenu);
    //End of file menu

    //Beginning of edit menu
    auto *editMenu = new QMenu(tr("Edit"));

    auto *copyAction = new QAction(QIcon::fromTheme("edit-copy"), tr("Copy"));
    connect(copyAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->copy();
    });
    editMenu->addAction(copyAction);

    auto *pasteAction = new QAction(QIcon::fromTheme("edit-paste"), tr("Paste"));
    connect(pasteAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->paste();
    });
    editMenu->addAction(pasteAction);

    menuBar->addMenu(editMenu);
    //End of edit menu

    //Beginning of view menu
    auto *viewMenu = new QMenu(tr("View"));

    auto *zoomInAction = new QAction(QIcon::fromTheme("zoom-in"), tr("Zoom In"));
    connect(zoomInAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->zoomIn();
    });
    viewMenu->addAction(zoomInAction);

    auto *zoomOutAction = new QAction(QIcon::fromTheme("zoom-out"), tr("Zoom Out"));
    connect(zoomOutAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->zoomOut();
    });
    viewMenu->addAction(zoomOutAction);

    auto *resetZoomAction = new QAction(QIcon::fromTheme("zoom-fit-best"), tr("Reset Zoom"));
    connect(resetZoomAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->resetZoom();
    });
    viewMenu->addAction(resetZoomAction);

    auto *originalSizeAction = new QAction(QIcon::fromTheme("zoom-original"), tr("Original Size"));
    connect(originalSizeAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->originalSize();
    });
    viewMenu->addAction(originalSizeAction);

    viewMenu->addSeparator();

    auto *rotateRightAction = new QAction(QIcon::fromTheme("object-rotate-right"), tr("Rotate Right"));
    connect(rotateRightAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->rotateRight();
    });
    viewMenu->addAction(rotateRightAction);

    auto *rotateLeftAction = new QAction(QIcon::fromTheme("object-rotate-left"), tr("Rotate Left"));
    connect(rotateLeftAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->rotateLeft();
    });
    viewMenu->addAction(rotateLeftAction);

    viewMenu->addSeparator();

    auto *mirrorAction = new QAction(QIcon::fromTheme("object-flip-horizontal"), tr("Mirror"));
    connect(mirrorAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->mirror();
    });
    viewMenu->addAction(mirrorAction);

    auto *flipAction = new QAction(QIcon::fromTheme("object-flip-vertical"), tr("Flip"));
    connect(flipAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->flip();
    });
    viewMenu->addAction(flipAction);

    menuBar->addMenu(viewMenu);
    //End of view menu

    //Beginning of go menu
    auto *goMenu = new QMenu(tr("Go"));

    auto *firstFileAction = new QAction(QIcon::fromTheme("go-first"), tr("First File"));
    connect(firstFileAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->firstFile();
    });
    goMenu->addAction(firstFileAction);

    auto *previousFileAction = new QAction(QIcon::fromTheme("go-previous"), tr("Previous File"));
    connect(previousFileAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->previousFile();
    });
    goMenu->addAction(previousFileAction);

    auto *nextFileAction = new QAction(QIcon::fromTheme("go-next"), tr("Next File"));
    connect(nextFileAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->nextFile();
    });
    goMenu->addAction(nextFileAction);

    auto *lastFileAction = new QAction(QIcon::fromTheme("go-last"), tr("Last File"));
    connect(lastFileAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->lastFile();
    });
    goMenu->addAction(lastFileAction);

    menuBar->addMenu(goMenu);
    //End of go menu

    //Beginning of tools menu
    auto *toolsMenu = new QMenu(tr("Tools"));

    auto *slideshowAction = new QAction(QIcon::fromTheme("media-playback-start"), tr("Start Slideshow"));
    toolsMenu->addAction(slideshowAction);

    auto *optionsAction = new QAction(QIcon::fromTheme("configure", QIcon::fromTheme("preferences-other")), tr("Options"));
    #if defined Q_OS_UNIX & !defined Q_OS_MACOS
        optionsAction->setText(tr("Preferences"));
    #elif defined Q_OS_MACOS
        optionsAction->setText(tr("Preferences..."));
    #endif
    connect(optionsAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->openOptions();
    });
    toolsMenu->addAction(optionsAction);

    menuBar->addMenu(toolsMenu);
    //End of tools menu

    //Beginning of help menu
    auto *helpMenu = new QMenu(tr("Help"));

    auto *aboutAction = new QAction(QIcon::fromTheme("help-about"), tr("About"));
    #if defined Q_OS_MACOS
        aboutAction->setText("About qView");
    #endif
    connect(aboutAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->openAbout();
    });
    helpMenu->addAction(aboutAction);

    auto *welcomeAction = new QAction(QIcon::fromTheme("help-faq", QIcon::fromTheme("help-about")), tr("Welcome"));
    connect(welcomeAction, &QAction::triggered, [](){
        if (auto *window = getCurrentMainWindow())
            window->openWelcome();
    });
    helpMenu->addAction(welcomeAction);

    menuBar->addMenu(helpMenu);
    //End of help menu

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
    auto activeWidget = activeWindow();

    if (activeWidget)
    {
        while (activeWidget->parentWidget() != nullptr)
            activeWidget = activeWidget->parentWidget();

        if (auto *window = qobject_cast<MainWindow*>(activeWidget))
        {
            return window;
        }
    }

    return nullptr;
}

MainWindow *QVApplication::getEmptyMainWindow()
{
    //Attempt to use the active window
    if (auto *window = getCurrentMainWindow())
    {
        if (!window->getIsPixmapLoaded())
        {
            return window;
        }
    }

    //If that is not valid, check all windows and use the first valid one
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

    //If there are no valid ones, make a new one.
    auto *window = newWindow();

    return window;
}

void QVApplication::updateDockRecents()
{
    QSettings settings;
    settings.beginGroup("options");

    bool isSaveRecentsEnabled = settings.value("saverecents", true).toBool();

    settings.endGroup();

    dockMenu->clear();
    if (isSaveRecentsEnabled) {
        settings.beginGroup("recents");
        QVariantList recentFiles = settings.value("recentFiles").value<QVariantList>();

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
        dockMenu->addSeparator();
    }
    auto *newWindowAction = new QAction(tr("New Window"));
    connect(newWindowAction, &QAction::triggered, []{
        newWindow();
    });
    auto *openAction = new QAction(tr("Open..."));
    connect(openAction, &QAction::triggered, []{
        pickFile();
    });
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

QHash<QString, QList<QKeySequence>> QVApplication::getShortcutsList()
{
    QSettings settings;
    settings.beginGroup("shortcuts");

    // To retrieve default bindings, we hackily init an options dialog and use it's constructor values
    QVOptionsDialog invisibleOptionsDialog;
    auto shortcutData = invisibleOptionsDialog.getTransientShortcuts();

    // Iterate through all default shortcuts to get saved shortcuts from settings
    QHash<QString, QList<QKeySequence>> shortcuts;
    QListIterator<QVShortcutDialog::SShortcut> iter(shortcutData);
    while (iter.hasNext())
    {
        auto value = iter.next();
        shortcuts.insert(value.name, QVOptionsDialog::stringListToKeySequenceList(settings.value(value.name, value.defaultShortcuts).value<QStringList>()));
    }
    return shortcuts;
}
