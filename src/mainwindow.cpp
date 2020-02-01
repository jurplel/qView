#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qvoptionsdialog.h"
#include "qvaboutdialog.h"
#include "qvwelcomedialog.h"
#include "qvapplication.h"
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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    //initialize variables
    windowResizeMode = 0;
    justLaunchedWithImage = false;
    maxWindowResizedPercentage = 0.7;

    //connect graphicsview signals
    connect(ui->graphicsView, &QVGraphicsView::fileLoaded, this, &MainWindow::fileLoaded);
    connect(ui->graphicsView, &QVGraphicsView::updatedLoadedPixmapItem, this, &MainWindow::setWindowSize);
    connect(ui->graphicsView, &QVGraphicsView::updatedFileInfo, this, &MainWindow::refreshProperties);
    connect(ui->graphicsView, &QVGraphicsView::updateRecentMenu, this, &MainWindow::updateRecentsMenu);
    connect(ui->graphicsView, &QVGraphicsView::sendWindowTitle, this, &MainWindow::setWindowTitle);
    connect(ui->graphicsView, &QVGraphicsView::cancelSlideshow, this, &MainWindow::cancelSlideshow);

    //Initialize escape shortcut
    escShortcut = new QShortcut(this);
    connect(escShortcut, &QShortcut::activated, this, [this](){
        if (windowState() == Qt::WindowFullScreen)
            toggleFullScreen();
    });

    //Enable drag&dropping
    setAcceptDrops(true);

    //Make info dialog object
    info = new QVInfoDialog(this);

    //Timer for slideshow
    slideshowTimer = new QTimer(this);
//    connect(slideshowTimer, &QTimer::timeout, this, &MainWindow::);

    //Load window geometry
    QSettings settings;
    settings.beginGroup("recents");
    restoreGeometry(settings.value("geometry").toByteArray());

    //Context menu
    auto *actionManager = qvApp->getActionManager();

    contextMenu = new QMenu(this);

    contextMenu->addAction(actionManager->getAction("open"));
    contextMenu->addAction(actionManager->getAction("openurl"));
    contextMenu->addMenu(actionManager->buildRecentsMenu());
    contextMenu->addAction(actionManager->getAction("opencontainingfolder"));
    contextMenu->addAction(actionManager->getAction("showfileinfo"));
    contextMenu->addSeparator();
    contextMenu->addAction(actionManager->getAction("copy"));
    contextMenu->addAction(actionManager->getAction("paste"));
    contextMenu->addSeparator();
    contextMenu->addAction(actionManager->getAction("nextfile"));
    contextMenu->addAction(actionManager->getAction("previousfile"));
    contextMenu->addSeparator();
    contextMenu->addMenu(actionManager->buildViewMenu());
    contextMenu->addMenu(actionManager->buildToolsMenu());
    contextMenu->addMenu(actionManager->buildHelpMenu());

//    //add actions not used in context menu so that keyboard shortcuts still work
//    addAction(ui->actionQuit);
//    addAction(ui->actionFirst_File);
//    addAction(ui->actionLast_File);

//    //Add to mainwindow's action list so keyboard shortcuts work without a menubar
//    foreach(QAction *action, contextMenu->actions())
//    {
//        addAction(action);
//    }

    loadSettings();
    updateRecentsMenu();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMainWindow::contextMenuEvent(event);
    updateRecentsMenu();
    contextMenu->popup(event->globalPos());
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    QSettings settings;
    settings.beginGroup("recents");

    if (settings.value("firstlaunch", false).toBool())
        return;

    settings.setValue("firstlaunch", true);
//    QTimer::singleShot(100, this, &MainWindow::on_actionWelcome_triggered);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.beginGroup("recents");
    settings.setValue("geometry", saveGeometry());
    QMainWindow::closeEvent(event);
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

void MainWindow::pickFile()
{
    QSettings settings;
    settings.beginGroup("recents");
    QFileDialog *fileDialog = new QFileDialog(this, tr("Open..."), "", tr("Supported Files (*.bmp *.cur *.gif *.icns *.ico *.jp2 *.jpeg *.jpe *.jpg *.mng *.pbm *.pgm *.png *.ppm *.svg *.svgz *.tif *.tiff *.wbmp *.webp *.xbm *.xpm);;All Files (*)"));
    fileDialog->setDirectory(settings.value("lastFileDialogDir", QDir::homePath()).toString());
    fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog->open();
    connect(fileDialog, &QFileDialog::filesSelected, [this](const QStringList &selected){
        openFile(selected.first());
        if (selected.length() > 1)
        {
            for (int i = 1; i < selected.length(); i++)
            {
                qDebug() << selected[i];
                QVApplication::newWindow(selected[i]);
            }
        }
    });
}

void MainWindow::openFile(const QString &fileName)
{
    QSettings settings;
    settings.beginGroup("recents");
    settings.setValue("lastFileDialogDir", QFileInfo(fileName).path());

    ui->graphicsView->loadFile(fileName);
    cancelSlideshow();
}


void MainWindow::loadSettings()
{
    QSettings settings;
    settings.beginGroup("options");
    //menubar
//    if (settings.value("menubarenabled", false).toBool())
//        ui->menuBar->show();
//    else
//        ui->menuBar->hide();

    //slideshowtimer
    slideshowTimer->setInterval(static_cast<int>(settings.value("slideshowtimer", 5).toDouble()*1000));

    //window resize mode
    windowResizeMode = settings.value("windowresizemode", 1).toInt();

    //max window resize mode size
    maxWindowResizedPercentage = settings.value("maxwindowresizedpercentage", 70).toReal()/100;

//    //saverecents
//    isSaveRecentsEnabled = settings.value("saverecents", true).toBool();
//    recentFilesMenu->menuAction()->setVisible(isSaveRecentsEnabled);
//    updateRecentsMenu();

//    ui->graphicsView->loadSettings();

//    loadShortcuts();
}

void MainWindow::loadShortcuts()
{
    auto shortcuts = qobject_cast<QVApplication*>(qApp)->getShortcutsList();

    // Set shortcuts by name from above list
//    ui->actionOpen->setShortcuts(shortcuts.value("open"));
//    ui->actionOpen_URL->setShortcuts(shortcuts.value("openurl"));
//    ui->actionOpen_Containing_Folder->setShortcuts(shortcuts.value("opencontainingfolder"));
//    ui->actionProperties->setShortcuts(shortcuts.value("showfileinfo"));
//    ui->actionCopy->setShortcuts(shortcuts.value("copy"));
//    ui->actionPaste->setShortcuts(shortcuts.value("paste"));
//    ui->actionFirst_File->setShortcuts(shortcuts.value("firstfile"));
//    ui->actionPrevious_File->setShortcuts(shortcuts.value("previousfile"));
//    ui->actionNext_File->setShortcuts(shortcuts.value("nextfile"));
//    ui->actionLast_File->setShortcuts(shortcuts.value("lastfile"));
//    ui->actionZoom_In->setShortcuts(shortcuts.value("zoomin"));
//    ui->actionZoom_Out->setShortcuts(shortcuts.value("zoomout"));
//    ui->actionReset_Zoom->setShortcuts(shortcuts.value("resetzoom"));
//    ui->actionOriginal_Size->setShortcuts(shortcuts.value("originalsize"));
//    ui->actionRotate_Right->setShortcuts(shortcuts.value("rotateright"));
//    ui->actionRotate_Left->setShortcuts(shortcuts.value("rotateleft"));
//    ui->actionMirror->setShortcuts(shortcuts.value("mirror"));
//    ui->actionFlip->setShortcuts(shortcuts.value("flip"));
//    ui->actionFull_Screen->setShortcuts(shortcuts.value("fullscreen"));
//    ui->actionSave_Frame_As->setShortcuts(shortcuts.value("saveframeas"));
//    ui->actionPause->setShortcuts(shortcuts.value("pause"));
//    ui->actionNext_Frame->setShortcuts(shortcuts.value("nextframe"));
//    ui->actionDecrease_Speed->setShortcuts(shortcuts.value("decreasespeed"));
//    ui->actionReset_Speed->setShortcuts(shortcuts.value("resetspeed"));
//    ui->actionIncrease_Speed->setShortcuts(shortcuts.value("increasespeed"));
//    ui->actionSlideshow->setShortcuts(shortcuts.value("slideshow"));
//    ui->actionOptions->setShortcuts(shortcuts.value("options"));
//    ui->actionNew_Window->setShortcuts(shortcuts.value("newwindow"));
//    ui->actionClose_Window->setShortcuts(shortcuts.value("closewindow"));
//    ui->actionQuit->setShortcuts(shortcuts.value("quit"));

    //Check if esc was used in a shortcut somewhere
    bool escUsed = false;
    foreach(auto sequenceList, shortcuts)
    {
        if (escUsed == true)
            break;

        if(sequenceList.contains(QKeySequence(Qt::Key_Escape)))
            escUsed = true;
    }
    if (escUsed)
    {
        escShortcut->setKey({});
    }
    else
    {
        escShortcut->setKey(Qt::Key_Escape);
    }
}

void MainWindow::updateRecentsMenu()
{
//    QSettings settings;
//    settings.beginGroup("recents");

//    //get recent files from config file
//    QVariantList recentFiles = settings.value("recentFiles").value<QVariantList>();

//    #ifdef Q_OS_MACX
//        qobject_cast<QVApplication*>(qApp)->updateDockRecents();
//    #endif
//    for (int i = 0; i <= 9; i++)
//    {
//        if (i < recentFiles.size())
//        {
//            recentItems[i]->setVisible(true);
//            recentItems[i]->setText(recentFiles[i].toList().first().toString());

//            //set menu icons for linux users
//            QMimeDatabase mimedb;
//            QMimeType type = mimedb.mimeTypeForFile(recentFiles[i].toList().last().toString());
//            if (type.iconName().isNull())
//                recentItems[i]->setIcon(QIcon::fromTheme(type.genericIconName()));
//            else
//                  recentItems[i]->setIcon(QIcon::fromTheme(type.iconName()));
//        }
//        else
//        {
//            recentItems[i]->setVisible(false);
//            recentItems[i]->setText(tr("Empty"));
//        }
//    }
}

void MainWindow::openRecent(int i)
{
    QSettings settings;
    settings.beginGroup("recents");
    QVariantList recentFiles = settings.value("recentFiles").value<QVariantList>();
    ui->graphicsView->loadFile(recentFiles[i].toList().last().toString());
    cancelSlideshow();
}

void MainWindow::clearRecent()
{
    QSettings settings;
    settings.beginGroup("recents");
    QVariantList recentFiles = settings.value("recentFiles").value<QVariantList>();
    for (int i = 0; i <= 9; i++)
    {
        if (recentFiles.size() > 1)
        {
            recentFiles.removeAt(1);
        }
        else
        {
            if (!ui->graphicsView->getCurrentFileDetails().isPixmapLoaded)
                recentFiles.removeAt(0);
        }
    }
    settings.setValue("recentFiles", recentFiles);

    updateRecentsMenu();
}

void MainWindow::cancelSlideshow()
{
//    if (slideshowTimer->isActive())
//        on_actionSlideshow_triggered();
}

void MainWindow::slideshowAction()
{
//    QSettings settings;
//    settings.beginGroup("options");
//    if(settings.value("slideshowdirection", 0).toInt() == 0)
//        on_actionNext_File_triggered();
//    else
//        on_actionPrevious_File_triggered();
}

void MainWindow::fileLoaded()
{
    //activate items after item is loaded for the first time
//    if (ui->graphicsView->getCurrentFileDetails().isPixmapLoaded && !ui->actionOpen_Containing_Folder->isEnabled())
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
//    ui->menuTools->actions().constFirst()->setEnabled(ui->graphicsView->getCurrentFileDetails().isMovieLoaded);
}

void MainWindow::refreshProperties()
{
    int value4;
    if (ui->graphicsView->getCurrentFileDetails().isMovieLoaded)
        value4 = ui->graphicsView->getLoadedMovie().frameCount();
    else
        value4 = 0;
    info->setInfo(ui->graphicsView->getCurrentFileDetails().fileInfo, ui->graphicsView->getCurrentFileDetails().imageSize.width(), ui->graphicsView->getCurrentFileDetails().imageSize.height(), value4);

}

void MainWindow::setWindowSize()
{
    //check if the program is configured to resize the window
    if (!(windowResizeMode == 2 || (windowResizeMode == 1 && justLaunchedWithImage)))
        return;

    //check if window is maximized or fullscreened
    if (windowState() == Qt::WindowMaximized || windowState() == Qt::WindowFullScreen)
        return;

    justLaunchedWithImage = false;

    QSize imageSize = ui->graphicsView->getCurrentFileDetails().loadedPixmapSize;
    imageSize -= QSize(4, 4);
    imageSize /= devicePixelRatioF();

    QSize currentScreenSize = screenAt(geometry().center())->size();
    currentScreenSize *= maxWindowResizedPercentage;

    if (imageSize.width() > currentScreenSize.width() || imageSize.height() > currentScreenSize.height())
        imageSize.scale(currentScreenSize, Qt::KeepAspectRatio);

    //Windows reports the wrong minimum width, so we constrain the image size relative to the dpi to stop weirdness with tiny images
    #ifdef Q_OS_WIN
    auto minimumImageSize = QSize(qRound(logicalDpiX()*1.5), logicalDpiY()/2);
    if (imageSize.boundedTo(minimumImageSize) == imageSize)
        imageSize = minimumImageSize;
    #endif

    QRect oldRect = geometry();
    resize(imageSize);
    QRect newRect = geometry();
    newRect.moveCenter(oldRect.center());

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

const bool& MainWindow::getIsPixmapLoaded() const
{
    return ui->graphicsView->getCurrentFileDetails().isPixmapLoaded;
}

void MainWindow::setJustLaunchedWithImage(bool value)
{
    justLaunchedWithImage = value;
}

void MainWindow::openUrl(QUrl url)
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
//    progressDialog->setWindowTitle(ui->actionOpen_URL->text());
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
//    inputDialog->setWindowTitle(ui->actionOpen_URL->text());
    inputDialog->setLabelText(tr("URL of a supported image file:"));
    inputDialog->resize(350, inputDialog->height());
    connect(inputDialog, &QInputDialog::finished, [inputDialog, this](int result) {
        if (!result)
            return;

        auto url = QUrl(inputDialog->textValue());
        inputDialog->deleteLater();
        openUrl(url);
    });
    inputDialog->open();
}

void MainWindow::openContainingFolder()
{
    if (!ui->graphicsView->getCurrentFileDetails().isPixmapLoaded)
        return;

    const QFileInfo selectedFileInfo = ui->graphicsView->getCurrentFileDetails().fileInfo;

    #if defined(Q_OS_WIN)
    QProcess::startDetached("explorer", QStringList() << "/select," << QDir::toNativeSeparators(selectedFileInfo.absoluteFilePath()));
    #elif defined(Q_OS_MACX)
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

void MainWindow::copy()
{
    auto *mimeData = ui->graphicsView->getMimeData();
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

    ui->graphicsView->loadMimeData(mimeData);
}

void MainWindow::zoomIn()
{
    ui->graphicsView->zoom(120, ui->graphicsView->mapFromGlobal(QCursor::pos()));
}

void MainWindow::zoomOut()
{
    ui->graphicsView->zoom(-120, ui->graphicsView->mapFromGlobal(QCursor::pos()));
}

void MainWindow::resetZoom()
{
    ui->graphicsView->resetScale();
}

void MainWindow::originalSize()
{
    ui->graphicsView->originalSize();
}

void MainWindow::rotateRight()
{
    ui->graphicsView->rotateImage(90);
    resetZoom();
}

void MainWindow::rotateLeft()
{
    ui->graphicsView->rotateImage(-90);
    resetZoom();
}

void MainWindow::mirror()
{
    ui->graphicsView->scale(-1, 1);
    resetZoom();
}

void MainWindow::flip()
{
    ui->graphicsView->scale(1, -1);
    resetZoom();
}

void MainWindow::firstFile()
{
    ui->graphicsView->goToFile(QVGraphicsView::goToFileMode::first);
}

void MainWindow::previousFile()
{
    ui->graphicsView->goToFile(QVGraphicsView::goToFileMode::previous);
}

void MainWindow::nextFile()
{
    ui->graphicsView->goToFile(QVGraphicsView::goToFileMode::next);
}

void MainWindow::lastFile()
{
    ui->graphicsView->goToFile(QVGraphicsView::goToFileMode::last);
}

void MainWindow::openOptions()
{
    auto *options = new QVOptionsDialog(this);
    options->open();

    connect(options, &QVOptionsDialog::optionsSaved, this, &MainWindow::loadSettings);
}

void MainWindow::openAbout()
{
    auto *about = new QVAboutDialog(this);
    about->exec();
}

void MainWindow::openWelcome()
{
    auto *welcome = new QVWelcomeDialog(this);
    welcome->exec();
}

void MainWindow::saveFrameAs()
{
    QSettings settings;
    settings.beginGroup("recents");
    if (!ui->graphicsView->getCurrentFileDetails().isMovieLoaded)
        return;

    if (ui->graphicsView->getLoadedMovie().state() == QMovie::Running)
    {
//        ui->actionPause->trigger();
    }
    QFileDialog *saveDialog = new QFileDialog(this, tr("Save Frame As..."), "", tr("Supported Files (*.bmp *.cur *.icns *.ico *.jp2 *.jpeg *.jpe *.jpg *.pbm *.pgm *.png *.ppm *.tif *.tiff *.wbmp *.webp *.xbm *.xpm);;All Files (*)"));
    saveDialog->setDirectory(settings.value("lastFileDialogDir", QDir::homePath()).toString());
    saveDialog->selectFile(ui->graphicsView->getCurrentFileDetails().fileInfo.baseName() + "-" + QString::number(ui->graphicsView->getLoadedMovie().currentFrameNumber()) + ".png");
    saveDialog->setDefaultSuffix("png");
    saveDialog->setAcceptMode(QFileDialog::AcceptSave);
    saveDialog->open();
    connect(saveDialog, &QFileDialog::fileSelected, this, [=](QString fileName){
        ui->graphicsView->originalSize();
        for(int i=0; i < ui->graphicsView->getLoadedMovie().frameCount(); i++)
//            ui->actionNext_Frame->trigger();

        ui->graphicsView->getLoadedMovie().currentPixmap().save(fileName, nullptr, 100);
        ui->graphicsView->resetScale();
    });
}

void MainWindow::pause()
{
    if (!ui->graphicsView->getCurrentFileDetails().isMovieLoaded)
        return;

    if (ui->graphicsView->getLoadedMovie().state() == QMovie::Running)
    {
        ui->graphicsView->setPaused(true);
//        ui->actionPause->setText(tr("Resume"));
//        ui->actionPause->setIcon(QIcon::fromTheme("media-playback-start"));
    }
    else
    {
        ui->graphicsView->setPaused(false);
//        ui->actionPause->setText(tr("Pause"));
//        ui->actionPause->setIcon(QIcon::fromTheme("media-playback-pause"));
    }
}

void MainWindow::nextFrame()
{
    if (!ui->graphicsView->getCurrentFileDetails().isMovieLoaded)
        return;

    ui->graphicsView->jumpToNextFrame();
}

void MainWindow::decreaseSpeed()
{
    if (!ui->graphicsView->getCurrentFileDetails().isMovieLoaded)
        return;

    ui->graphicsView->setSpeed(ui->graphicsView->getLoadedMovie().speed()-25);
}

void MainWindow::resetSpeed()
{
    if (!ui->graphicsView->getCurrentFileDetails().isMovieLoaded)
        return;

    ui->graphicsView->setSpeed(100);
}

void MainWindow::increaseSpeed()
{
    if (!ui->graphicsView->getCurrentFileDetails().isMovieLoaded)
        return;

    ui->graphicsView->setSpeed(ui->graphicsView->getLoadedMovie().speed()+25);
}

void MainWindow::toggleFullScreen()
{
    if (windowState() == Qt::WindowFullScreen)
        showNormal();
    else
        showFullScreen();
}
