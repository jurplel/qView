#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qvapplication.h"
#include "qvcocoafunctions.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QClipboard>
#include <QCoreApplication>
#include <QFileSystemWatcher>
#include <QProcess>
#include <QDesktopServices>
#include <QContextMenuEvent>
#include <QMovie>
#include <QImageWriter>
#include <QSettings>
#include <QStyle>
#include <QIcon>
#include <QMimeDatabase>
#include <QScreen>
#include <QCursor>
#include <QInputDialog>
#include <QProgressDialog>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QMenu>
#include <QWindow>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTemporaryFile>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    // Initialize variables
    justLaunchedWithImage = false;

    // Initialize graphicsview
    graphicsView = new QVGraphicsView(this);
    centralWidget()->layout()->addWidget(graphicsView);

    // Connect graphicsview signals
    connect(graphicsView, &QVGraphicsView::fileLoaded, this, &MainWindow::fileLoaded);
    connect(graphicsView, &QVGraphicsView::updatedLoadedPixmapItem, this, &MainWindow::setWindowSize);
    connect(graphicsView, &QVGraphicsView::cancelSlideshow, this, &MainWindow::cancelSlideshow);

    // Initialize escape shortcut
    escShortcut = new QShortcut(Qt::Key_Escape, this);
    connect(escShortcut, &QShortcut::activated, this, [this](){
        if (windowState() == Qt::WindowFullScreen)
            toggleFullScreen();
    });

    // Enable drag&dropping
    setAcceptDrops(true);

    // Make info dialog object
    info = new QVInfoDialog(this);

    // Timer for slideshow
    slideshowTimer = new QTimer(this);
    connect(slideshowTimer, &QTimer::timeout, this, &MainWindow::slideshowAction);

    // Load window geometry
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());

    // Context menu
    auto &actionManager = qvApp->getActionManager();

    contextMenu = new QMenu(this);

    contextMenu->addAction(actionManager.cloneAction("open"));
    contextMenu->addAction(actionManager.cloneAction("openurl"));
    contextMenu->addMenu(actionManager.buildRecentsMenu(true, contextMenu));
    contextMenu->addAction(actionManager.cloneAction("opencontainingfolder"));
    contextMenu->addAction(actionManager.cloneAction("showfileinfo"));
    contextMenu->addSeparator();
    contextMenu->addAction(actionManager.cloneAction("deletefile"));
    contextMenu->addSeparator();
    contextMenu->addAction(actionManager.cloneAction("copy"));
    contextMenu->addAction(actionManager.cloneAction("paste"));
    contextMenu->addAction(actionManager.cloneAction("rename"));
    contextMenu->addSeparator();
    contextMenu->addAction(actionManager.cloneAction("nextfile"));
    contextMenu->addAction(actionManager.cloneAction("previousfile"));
    contextMenu->addSeparator();
    contextMenu->addMenu(actionManager.buildViewMenu(true, contextMenu));
    contextMenu->addMenu(actionManager.buildToolsMenu(true, contextMenu));
    contextMenu->addMenu(actionManager.buildHelpMenu(true, contextMenu));

    connect(contextMenu, &QMenu::triggered, [this](QAction *triggeredAction){
        ActionManager::actionTriggered(triggeredAction, this);
    });

    // Initialize menubar
    setMenuBar(actionManager.buildMenuBar(this));
    connect(menuBar(), &QMenuBar::triggered, [this](QAction *triggeredAction){
        ActionManager::actionTriggered(triggeredAction, this);
    });

    // Add all actions to this window so keyboard shortcuts are always triggered
    // using virtual menu to hold them so i can connect to the triggered signal
    virtualMenu = new QMenu(this);
    virtualMenu->addActions(actionManager.getActionLibrary().values());
    addActions(virtualMenu->actions());
    connect(virtualMenu, &QMenu::triggered, [this](QAction *triggeredAction){
       ActionManager::actionTriggered(triggeredAction, this);
    });

    // Connect functions to application components
    connect(&qvApp->getShortcutManager(), &ShortcutManager::shortcutsUpdated, this, &MainWindow::shortcutsUpdated);
    connect(&qvApp->getSettingsManager(), &SettingsManager::settingsUpdated, this, &MainWindow::settingsUpdated);
    settingsUpdated();
    shortcutsUpdated();
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate)
    {
        qvApp->addToLastActiveWindows(this);
    }
    return QMainWindow::event(event);
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMainWindow::contextMenuEvent(event);

    // Show native menu on macOS
#ifdef Q_OS_MACOS
    QVCocoaFunctions::showMenu(contextMenu, event->pos(), windowHandle());
#else
    contextMenu->popup(event->globalPos());
#endif
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());

    qvApp->deleteFromLastActiveWindows(this);
    qvApp->getActionManager().untrackClonedActions(contextMenu);
    qvApp->getActionManager().untrackClonedActions(menuBar());

    QMainWindow::closeEvent(event);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        const auto fullscreenActions = qvApp->getActionManager().getAllInstancesOfAction("fullscreen");
        for (const auto &fullscreenAction : fullscreenActions)
        {
                if (windowState() == Qt::WindowFullScreen)
                {
                    fullscreenAction->setText(tr("Exit Full Screen"));
                    fullscreenAction->setIcon(QIcon::fromTheme("view-restore"));
                }
                else
                {
                    fullscreenAction->setText(tr("Enter Full Screen"));
                    fullscreenAction->setIcon(QIcon::fromTheme("view-fullscreen"));
                }
            QTimer::singleShot(3000, this, [fullscreenAction]{
                fullscreenAction->setVisible(true);
             });
        }
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::BackButton)
        previousFile();
    else if (event->button() == Qt::MouseButton::ForwardButton)
        nextFile();

    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
        toggleFullScreen();
    QMainWindow::mouseDoubleClickEvent(event);
}

void MainWindow::openFile(const QString &fileName)
{
    QUrl url(fileName);
    graphicsView->loadFile(fileName);
    cancelSlideshow();
}

void MainWindow::settingsUpdated()
{
    auto &settingsManager = qvApp->getSettingsManager();

    buildWindowTitle();

    // menubarenabled
    bool menuBarEnabled = settingsManager.getBoolean("menubarenabled");
#ifdef Q_OS_MACOS
    // Menu bar is effectively always enabled on macOS
    menuBarEnabled = true;
#endif
    menuBar()->setVisible(menuBarEnabled);
    const auto actionList = actions();
    for (const auto &action : actionList)
    {
        action->setVisible(!menuBarEnabled);
    }

    // titlebaralwaysdark
#ifdef Q_OS_MACOS
    QVCocoaFunctions::setVibrancy(settingsManager.getBoolean("titlebaralwaysdark"), windowHandle());
#endif

    //slideshow timer
    slideshowTimer->setInterval(static_cast<int>(settingsManager.getDouble("slideshowtimer")*1000));

}

void MainWindow::shortcutsUpdated()
{
    // If esc is not used in a shortcut, let it exit fullscreen
    escShortcut->setKey(Qt::Key_Escape);

    const auto &actionLibrary = qvApp->getActionManager().getActionLibrary();
    for (const auto &action : actionLibrary)
    {
        if (action->shortcuts().contains(QKeySequence(Qt::Key_Escape)))
        {
            escShortcut->setKey({});
            break;
        }
    }
}

void MainWindow::openRecent(int i)
{
    auto recentsList = qvApp->getActionManager().getRecentsList();
    graphicsView->loadFile(recentsList.value(i).filePath);
    cancelSlideshow();
}

void MainWindow::fileLoaded()
{
    //activate items after item is loaded for the first time
//    if (getCurrentFileDetails().isPixmapLoaded && !ui->actionOpen_Containing_Folder->isEnabled())
//    {
//        foreach(QAction* action, ui->menuView->actions())
//            action->setEnabled(true);
//        foreach(QAction* action, contextMenu->actions())
//            action->setEnabled(true);
//        foreach(QAction* action, actions())
//            action->setEnabled(true);
//        ui->actionSlideshow->setEnabled(true);
//        ui->actionCopy->setEnabled(true);
//    }
    //disable gif controls if there is no gif loaded
//    ui->menuTools->actions().constFirst()->setEnabled(getCurrentFileDetails().isMovieLoaded);

    refreshProperties();
    buildWindowTitle();
}

void MainWindow::refreshProperties()
{
    int value4;
    if (getCurrentFileDetails().isMovieLoaded)
        value4 = graphicsView->getLoadedMovie().frameCount();
    else
        value4 = 0;
    info->setInfo(getCurrentFileDetails().fileInfo, getCurrentFileDetails().baseImageSize.width(), getCurrentFileDetails().baseImageSize.height(), value4);
}

void MainWindow::buildWindowTitle()
{
    if (!getCurrentFileDetails().isPixmapLoaded)
        return;

    QString newString;
    switch (qvApp->getSettingsManager().getInteger("titlebarmode")) {
    case 0:
    {
        newString = "qView";
        break;
    }
    case 1:
    {
        newString = getCurrentFileDetails().fileInfo.fileName();
        break;
    }
    case 2:
    {
        newString += QString::number(getCurrentFileDetails().loadedIndexInFolder+1);
        newString += "/" + QString::number(getCurrentFileDetails().folderFileInfoList.count());
        newString += " - " + getCurrentFileDetails().fileInfo.fileName();
        break;
    }
    case 3:
    {
        newString += QString::number(getCurrentFileDetails().loadedIndexInFolder+1);
        newString += "/" + QString::number(getCurrentFileDetails().folderFileInfoList.count());
        newString += " - " + getCurrentFileDetails().fileInfo.fileName();
        newString += " - "  + QString::number(getCurrentFileDetails().baseImageSize.width());
        newString += "x" + QString::number(getCurrentFileDetails().baseImageSize.height());
        newString += " - " + QVInfoDialog::formatBytes(getCurrentFileDetails().fileInfo.size());
        newString += " - qView";
        break;
    }
    }

    setWindowTitle(newString);
    windowHandle()->setFilePath(getCurrentFileDetails().fileInfo.absoluteFilePath());
}

void MainWindow::setWindowSize()
{
    int windowResizeMode = qvApp->getSettingsManager().getInteger("windowresizemode");
    qreal minWindowResizedPercentage = qvApp->getSettingsManager().getInteger("minwindowresizedpercentage")/100.0;
    qreal maxWindowResizedPercentage = qvApp->getSettingsManager().getInteger("maxwindowresizedpercentage")/100.0;

    //check if the program is configured to resize the window
    if (!(windowResizeMode == 2 || (windowResizeMode == 1 && justLaunchedWithImage)))
        return;

    //check if window is maximized or fullscreened
    if (windowState() == Qt::WindowMaximized || windowState() == Qt::WindowFullScreen)
        return;

    justLaunchedWithImage = false;

    QSize imageSize = getCurrentFileDetails().loadedPixmapSize;
    imageSize -= QSize(4, 4);
    imageSize /= devicePixelRatioF();

    QSize currentScreenSize = screenAt(geometry().center())->size();

    QSize minWindowSize = currentScreenSize * minWindowResizedPercentage;
    QSize maxWindowSize = currentScreenSize * maxWindowResizedPercentage;

    if (imageSize.width() < minWindowSize.width() || imageSize.height() < minWindowSize.height())
    {
        imageSize.scale(minWindowSize, Qt::KeepAspectRatio);
    }
    else if (imageSize.width() > maxWindowSize.width() || imageSize.height() > maxWindowSize.height())
    {
        imageSize.scale(maxWindowSize, Qt::KeepAspectRatio);
    }

    // Windows reports the wrong minimum width, so we constrain the image size relative to the dpi to stop weirdness with tiny images
#ifdef Q_OS_WIN
    auto minimumImageSize = QSize(qRound(logicalDpiX()*1.5), logicalDpiY()/2);
    if (imageSize.boundedTo(minimumImageSize) == imageSize)
        imageSize = minimumImageSize;
#endif

    // Adjust image size for fullsizecontentview on mac
#ifdef Q_OS_MACOS
    int obscuredHeight = QVCocoaFunctions::getObscuredHeight(window()->windowHandle());
    imageSize.setHeight(imageSize.height() + obscuredHeight);
#endif

    // Match center after new geometry
    QRect oldRect = geometry();
    resize(imageSize);
    QRect newRect = geometry();
    newRect.moveCenter(oldRect.center());

    // Ensure titlebar is not above the top of the screen
    const int titlebarHeight = QApplication::style()->pixelMetric(QStyle::PM_TitleBarHeight);
    if (newRect.y() < titlebarHeight)
        newRect.setY(titlebarHeight);


    setGeometry(newRect);
}

//literally just copy pasted from Qt source code to maintain compatibility with 5.9
QScreen *MainWindow::screenAt(const QPoint &point)
{
    QVarLengthArray<const QScreen *, 8> visitedScreens;
    for (const QScreen *screen : QGuiApplication::screens()) {
        if (visitedScreens.contains(screen))
            continue;
        // The virtual siblings include the screen itself, so iterate directly
        for (QScreen *sibling : screen->virtualSiblings()) {
            if (sibling->geometry().contains(point))
                return sibling;
            visitedScreens.append(sibling);
        }
    }
    return nullptr;
}

bool MainWindow::getIsPixmapLoaded() const
{
    return getCurrentFileDetails().isPixmapLoaded;
}

void MainWindow::setJustLaunchedWithImage(bool value)
{
    justLaunchedWithImage = value;
}

void MainWindow::openUrl(const QUrl &url)
{
    auto *networkManager = new QNetworkAccessManager(this);

    if (!url.isValid()) {
        QMessageBox::critical(this, tr("Error"), tr("Error: URL is invalid"));
        return;
    }

    auto request = QNetworkRequest(url);
    auto *reply = networkManager->get(request);
    auto *progressDialog = new QProgressDialog(tr("Downloading image..."), tr("Cancel"), 0, 100);
    progressDialog->setAutoClose(false);
    progressDialog->setAutoReset(false);
    progressDialog->setWindowTitle(tr("Open URL..."));
    progressDialog->open();

    connect(progressDialog, &QProgressDialog::canceled, [reply]{
        reply->abort();
    });

    connect(reply, &QNetworkReply::downloadProgress, [progressDialog](qreal bytesReceived, qreal bytesTotal){
        auto percent = (bytesReceived/bytesTotal)*100;
        progressDialog->setValue(qRound(percent));
    });


    connect(reply, &QNetworkReply::finished, [networkManager, progressDialog, reply, this]{
        if (reply->error())
        {
            progressDialog->close();
            QMessageBox::critical(this, tr("Error"), tr("Error ") + QString::number(reply->error()) + ": " + reply->errorString());

            networkManager->deleteLater();
            progressDialog->deleteLater();
            return;
        }

        progressDialog->setMaximum(0);

        auto *tempFile = new QTemporaryFile(this);

        auto *saveFutureWatcher = new QFutureWatcher<bool>();
        connect(saveFutureWatcher, &QFutureWatcher<bool>::finished, this, [networkManager, progressDialog, tempFile, saveFutureWatcher, this](){
            progressDialog->close();
            if (saveFutureWatcher->result())
            {
                if (tempFile->open())
                {
                    openFile(tempFile->fileName());
                }
            }
            else
            {
                 QMessageBox::critical(this, tr("Error"), tr("Error: Invalid image"));
                 tempFile->deleteLater();
            }
            progressDialog->deleteLater();
            networkManager->deleteLater();
            saveFutureWatcher->deleteLater();
        });

        saveFutureWatcher->setFuture(QtConcurrent::run([reply, tempFile]{
            return QImage::fromData(reply->readAll()).save(tempFile, "png");
        }));
    });
}

void MainWindow::pickUrl()
{
    auto inputDialog = new QInputDialog(this);
    inputDialog->setWindowTitle(tr("Open URL..."));
    inputDialog->setLabelText(tr("URL of a supported image file:"));
    inputDialog->resize(350, inputDialog->height());
    connect(inputDialog, &QInputDialog::finished, [inputDialog, this](int result) {
        if (result)
        {
            const auto url = QUrl(inputDialog->textValue());
            openUrl(url);
        }
        inputDialog->deleteLater();
    });
    inputDialog->open();
}

void MainWindow::openContainingFolder()
{
    if (!getCurrentFileDetails().isPixmapLoaded)
        return;

    const QFileInfo selectedFileInfo = getCurrentFileDetails().fileInfo;

#ifdef Q_OS_WIN
    QProcess::startDetached("explorer", QStringList() << "/select," << QDir::toNativeSeparators(selectedFileInfo.absoluteFilePath()));
#elif defined Q_OS_MACOS
    QProcess::execute("open", QStringList() << "-R" << selectedFileInfo.absoluteFilePath());
#else
    QDesktopServices::openUrl(selectedFileInfo.absolutePath());
#endif
}

void MainWindow::showFileInfo()
{
    refreshProperties();
    info->show();
}

void MainWindow::deleteFile()
{
    const QString filepath = getCurrentFileDetails().fileInfo.absoluteFilePath();

    nextFile();

#ifdef Q_OS_WIN
    /// todo Delete file to trash on Windows

    QFile::remove(filepath);
#elif defined Q_OS_MACOS
    QString param = "tell app \"Finder\" to delete POSIX file ";
    param += "\"" + filepath + "\"";

    const QStringList arguments = { "-e", param };

    QProcess::execute("osascript", arguments);
#elif defined Q_OS_LINUX
    /// todo Test delete file to trash on Linux

    QString param = "\"file:";
    param += filepath;
    param += "\"";

    const QStringList arguments = { "trash", param };

    QProcess::execute("gio", arguments);
#else
#endif
}

void MainWindow::copy()
{
    auto *mimeData = graphicsView->getMimeData();
    if (!mimeData->hasImage() || !mimeData->hasUrls())
    {
        mimeData->deleteLater();
        return;
    }

    QApplication::clipboard()->setMimeData(mimeData);
}

void MainWindow::paste()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();

    if (mimeData->hasText())
    {
        auto url = QUrl(mimeData->text());

        if (url.isValid() && (url.scheme() == "http" || url.scheme() == "https"))
        {
            openUrl(url);
            return;
        }
    }

    graphicsView->loadMimeData(mimeData);
}

void MainWindow::rename()
{
    if (!getCurrentFileDetails().isPixmapLoaded)
        return;

    auto currentFileInfo = getCurrentFileDetails().fileInfo;

    auto *renameDialog = new QInputDialog(this);
    renameDialog->setWindowTitle(tr("Rename..."));
    renameDialog->setLabelText(tr("File name:"));
    renameDialog->setTextValue(currentFileInfo.fileName());
    renameDialog->resize(350, renameDialog->height());
    connect(renameDialog, &QInputDialog::finished, [currentFileInfo, renameDialog, this](int result) {
        if (result)
        {
            const auto newFileName = renameDialog->textValue();
            const auto newFilePath = QDir::cleanPath(currentFileInfo.absolutePath() + QDir::separator() + newFileName);

            if (currentFileInfo.absoluteFilePath() != newFilePath)
            {
                if (QFile::rename(currentFileInfo.absoluteFilePath(), newFilePath))
                {
                    openFile(newFilePath);
                }
                else
                {
                    QMessageBox::critical(this, tr("Error"), tr("Error: Could not rename file\n(Check that you have write access)"));
                }
            }
        }

        renameDialog->deleteLater();
    });
    renameDialog->open();
}

void MainWindow::zoomIn()
{
    graphicsView->zoom(120, graphicsView->mapFromGlobal(QCursor::pos()));
}

void MainWindow::zoomOut()
{
    graphicsView->zoom(-120, graphicsView->mapFromGlobal(QCursor::pos()));
}

void MainWindow::resetZoom()
{
    graphicsView->resetScale();
}

void MainWindow::originalSize()
{
    graphicsView->originalSize();
}

void MainWindow::rotateRight()
{
    graphicsView->rotateImage(90);
    resetZoom();
}

void MainWindow::rotateLeft()
{
    graphicsView->rotateImage(-90);
    resetZoom();
}

void MainWindow::mirror()
{
    graphicsView->scale(-1, 1);
    resetZoom();
}

void MainWindow::flip()
{
    graphicsView->scale(1, -1);
    resetZoom();
}

void MainWindow::firstFile()
{
    graphicsView->goToFile(QVGraphicsView::GoToFileMode::first);
}

void MainWindow::previousFile()
{
    graphicsView->goToFile(QVGraphicsView::GoToFileMode::previous);
}

void MainWindow::nextFile()
{
    graphicsView->goToFile(QVGraphicsView::GoToFileMode::next);
}

void MainWindow::lastFile()
{
    graphicsView->goToFile(QVGraphicsView::GoToFileMode::last);
}

void MainWindow::saveFrameAs()
{
    QSettings settings;
    settings.beginGroup("recents");
    if (!getCurrentFileDetails().isMovieLoaded)
        return;

    if (graphicsView->getLoadedMovie().state() == QMovie::Running)
    {
        pause();
    }
    QFileDialog *saveDialog = new QFileDialog(this, tr("Save Frame As..."));
    saveDialog->setDirectory(settings.value("lastFileDialogDir", QDir::homePath()).toString());
    saveDialog->setNameFilters(qvApp->getNameFilterList());
    saveDialog->selectFile(getCurrentFileDetails().fileInfo.baseName() + "-" + QString::number(graphicsView->getLoadedMovie().currentFrameNumber()) + ".png");
    saveDialog->setDefaultSuffix("png");
    saveDialog->setAcceptMode(QFileDialog::AcceptSave);
    saveDialog->open();
    connect(saveDialog, &QFileDialog::fileSelected, this, [=](const QString &fileName){
        graphicsView->originalSize();
        for(int i=0; i < graphicsView->getLoadedMovie().frameCount(); i++)
            nextFrame();

        graphicsView->getLoadedMovie().currentPixmap().save(fileName, nullptr, 100);
        graphicsView->resetScale();
    });
}

void MainWindow::pause()
{
    if (!getCurrentFileDetails().isMovieLoaded)
        return;

    const auto pauseActions = qvApp->getActionManager().getAllInstancesOfAction("pause");

    if (graphicsView->getLoadedMovie().state() == QMovie::Running)
    {
        graphicsView->setPaused(true);
        for (const auto &pauseAction : pauseActions)
        {
            pauseAction->setText(tr("Resume"));
            pauseAction->setIcon(QIcon::fromTheme("media-playback-start"));
        }
    }
    else
    {
        graphicsView->setPaused(false);
        for (const auto &pauseAction : pauseActions)
        {
            pauseAction->setText(tr("Pause"));
            pauseAction->setIcon(QIcon::fromTheme("media-playback-pause"));
        }
    }
}

void MainWindow::nextFrame()
{
    if (!getCurrentFileDetails().isMovieLoaded)
        return;

    graphicsView->jumpToNextFrame();
}

void MainWindow::toggleSlideshow()
{
    const auto slideshowActions = qvApp->getActionManager().getAllInstancesOfAction("slideshow");

    if (slideshowTimer->isActive())
    {
        slideshowTimer->stop();
        for (const auto &slideshowAction : slideshowActions)
        {
            slideshowAction->setText(tr("Start Slideshow"));
            slideshowAction->setIcon(QIcon::fromTheme("media-playback-start"));
        }
    }
    else
    {
        slideshowTimer->start();
        for (const auto &slideshowAction : slideshowActions)
        {
            slideshowAction->setText(tr("Stop Slideshow"));
            slideshowAction->setIcon(QIcon::fromTheme("media-playback-stop"));
        }
    }
}

void MainWindow::cancelSlideshow()
{
    if (slideshowTimer->isActive())
        toggleSlideshow();
}

void MainWindow::slideshowAction()
{
    if (qvApp->getSettingsManager().getBoolean("slideshowreversed"))
        previousFile();
    else
        nextFile();
}

void MainWindow::decreaseSpeed()
{
    if (!getCurrentFileDetails().isMovieLoaded)
        return;

    graphicsView->setSpeed(graphicsView->getLoadedMovie().speed()-25);
}

void MainWindow::resetSpeed()
{
    if (!getCurrentFileDetails().isMovieLoaded)
        return;

    graphicsView->setSpeed(100);
}

void MainWindow::increaseSpeed()
{
    if (!getCurrentFileDetails().isMovieLoaded)
        return;

    graphicsView->setSpeed(graphicsView->getLoadedMovie().speed()+25);
}

void MainWindow::toggleFullScreen()
{
    if (windowState() == Qt::WindowFullScreen)
    {
        showNormal();
    }
    else
    {
        showFullScreen();
    }
}
