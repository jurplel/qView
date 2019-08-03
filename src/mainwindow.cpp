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
#include <QShortcut>
#include <QScreen>
#include <QCursor>
#include <QInputDialog>
#include <QProgressDialog>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //initialize variables
    windowResizeMode = 0;
    justLaunchedWithImage = false;
    maxWindowResizedPercentage = 0.7;

    //connect graphicsview signals
    connect(ui->graphicsView, &QVGraphicsView::fileLoaded, this, &MainWindow::fileLoaded);
    connect(ui->graphicsView, &QVGraphicsView::updatedLoadedPixmapItem, this, &MainWindow::setWindowSize);
    connect(ui->graphicsView, &QVGraphicsView::updatedFileInfo, this, &MainWindow::refreshProperties);
    connect(ui->graphicsView, &QVGraphicsView::updateRecentMenu, this, &MainWindow::updateRecentMenu);
    connect(ui->graphicsView, &QVGraphicsView::sendWindowTitle, this, &MainWindow::setWindowTitle);
    connect(ui->graphicsView, &QVGraphicsView::cancelSlideshow, this, &MainWindow::cancelSlideshow);

    //Enable drag&dropping
    setAcceptDrops(true);

    //Make info dialog
    info = new QVInfoDialog(this);

    //Timer for slideshow
    slideshowTimer = new QTimer(this);
    connect(slideshowTimer, &QTimer::timeout, this, &MainWindow::slideshowAction);

    //Load settings from file
    loadSettings();
    QSettings settings;
    settings.beginGroup("recents");
    restoreGeometry(settings.value("geometry").toByteArray());

    //Esc to exit fullscreen
    auto *escShortcut = new QShortcut(this);
    escShortcut->setKey(Qt::Key_Escape);
    connect(escShortcut, &QShortcut::activated, this, [this](){
        if (windowState() == Qt::WindowFullScreen)
            showNormal();
    });

    //Context menu
    contextMenu = new QMenu(this);

    contextMenu->addAction(ui->actionOpen);
    contextMenu->addAction(ui->actionOpen_URL);

    QMenu *files = new QMenu(tr("Open Recent"), this);

    int index = 0;

    for ( int i = 0; i <= 9; i++ )
    {
        recentItems.append(new QAction(tr("Empty"), this));
    }

    foreach(QAction *action, recentItems)
    {
        connect(action, &QAction::triggered, this, [this,index]() { openRecent(index); });
        files->addAction(action);
        index++;
    }
    files->addSeparator();
    QAction *clearMenu = new QAction(tr("Clear Menu"), this);
    connect(clearMenu, &QAction::triggered, this, &MainWindow::clearRecent);
    files->addAction(clearMenu);

    contextMenu->addMenu(files);

    contextMenu->addAction(ui->actionOpen_Containing_Folder);
    contextMenu->addAction(ui->actionProperties);
    contextMenu->addSeparator();
    contextMenu->addAction(ui->actionCopy);
    contextMenu->addAction(ui->actionPaste);
    contextMenu->addSeparator();
    contextMenu->addAction(ui->actionPrevious_File);
    contextMenu->addAction(ui->actionNext_File);
    contextMenu->addSeparator();

    QMenu *view = new QMenu(tr("View"), this);
    view->menuAction()->setEnabled(false);
    view->addAction(ui->actionZoom_In);
    view->addAction(ui->actionZoom_Out);
    view->addAction(ui->actionReset_Zoom);
    view->addAction(ui->actionOriginal_Size);
    view->addSeparator();
    view->addAction(ui->actionRotate_Right);
    view->addAction(ui->actionRotate_Left);
    view->addSeparator();
    view->addAction(ui->actionMirror);
    view->addAction(ui->actionFlip);
    view->addSeparator();
    view->addAction(ui->actionFull_Screen);
    contextMenu->addMenu(view);

    QMenu *gif = new QMenu(tr("GIF Controls"), this);
    gif->menuAction()->setEnabled(false);
    gif->addAction(ui->actionSave_Frame_As);
    gif->addAction(ui->actionPause);
    gif->addAction(ui->actionNext_Frame);
    gif->addSeparator();
    gif->addAction(ui->actionDecrease_Speed);
    gif->addAction(ui->actionReset_Speed);
    gif->addAction(ui->actionIncrease_Speed);

    QMenu *tools = new QMenu(tr("Tools"), this);
    tools->addMenu(gif);
    tools->addAction(ui->actionSlideshow);
    tools->addAction(ui->actionOptions);
    contextMenu->addMenu(tools);

    QMenu *help = new QMenu(tr("Help"), this);
    help->addAction(ui->actionAbout);
    help->addAction(ui->actionWelcome);
    contextMenu->addMenu(help);

    //Menu icons that can't be set in the ui file
    files->setIcon(QIcon::fromTheme("document-open-recent"));
    clearMenu->setIcon(QIcon::fromTheme("edit-delete"));
    view->setIcon(QIcon::fromTheme("zoom-fit-best"));
    tools->setIcon(QIcon::fromTheme("configure", QIcon::fromTheme("preferences-other")));
    gif->setIcon(QIcon::fromTheme("media-playlist-repeat"));
    help->setIcon(QIcon::fromTheme("help-about"));

    //fallback icons
    ui->actionWelcome->setIcon(QIcon::fromTheme("help-faq", QIcon::fromTheme("help-about")));
    ui->actionOptions->setIcon(QIcon::fromTheme("configure", QIcon::fromTheme("preferences-other")));
    ui->actionOpen_URL->setIcon(QIcon::fromTheme("document-open-remote", QIcon::fromTheme("folder-remote")));

    //Add recent items to menubar
    ui->menuFile->insertMenu(ui->actionOpen_Containing_Folder, files);
    ui->menuFile->insertSeparator(ui->actionOpen_Containing_Folder);
    ui->menuTools->insertMenu(ui->actionSlideshow, gif);

    //add actions not used in context menu so that keyboard shortcuts still work
    addAction(ui->actionQuit);
    addAction(ui->actionFirst_File);
    addAction(ui->actionLast_File);

    //macOS specific functions
    #ifdef Q_OS_UNIX
    ui->actionOptions->setText(tr("Preferences"));
    #endif
    #ifdef Q_OS_MACX
    ui->actionAbout->setText(tr("About qView"));
    ui->actionOptions->setText(tr("Preferences..."));
    ui->menuView->removeAction(ui->actionFull_Screen);
    ui->actionNew_Window->setVisible(true);
    ui->actionOpen_Containing_Folder->setText(tr("Show in Finder"));

    //macOS dock menu
    dockMenu = new QMenu(this);
    dockMenu->setAsDockMenu();
    dockMenu->addAction(ui->actionNew_Window);
    dockMenu->addAction(ui->actionOpen);
    #elif defined(Q_OS_WIN)
    ui->actionOpen_Containing_Folder->setText(tr("Show in Explorer"));
    #endif

    //Add to mainwindow's action list so keyboard shortcuts work without a menubar
    foreach(QAction *action, contextMenu->actions())
    {
        addAction(action);
    }

    updateRecentMenu();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMainWindow::contextMenuEvent(event);
    updateRecentMenu();
    contextMenu->exec(event->globalPos());
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    QSettings settings;
    settings.beginGroup("recents");

    if (settings.value("firstlaunch", false).toBool())
        return;

    settings.setValue("firstlaunch", true);
    QTimer::singleShot(100, this, &MainWindow::on_actionWelcome_triggered);
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
        on_actionPrevious_File_triggered();
    else if (event->button() == Qt::MouseButton::ForwardButton)
        on_actionNext_File_triggered();

    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
        on_actionFull_Screen_triggered();
    QMainWindow::mouseDoubleClickEvent(event);
}

void MainWindow::pickFile()
{
    QSettings settings;
    settings.beginGroup("recents");
    QFileDialog *fileDialog = new QFileDialog(this, ui->actionOpen->text(), "", tr("Supported Files (*.bmp *.cur *.gif *.icns *.ico *.jp2 *.jpeg *.jpe *.jpg *.mng *.pbm *.pgm *.png *.ppm *.svg *.svgz *.tif *.tiff *.wbmp *.webp *.xbm *.xpm);;All Files (*)"));
    fileDialog->setDirectory(settings.value("lastFileDialogDir", QDir::homePath()).toString());
    fileDialog->open();
    connect(fileDialog, &QFileDialog::fileSelected, this, &MainWindow::openFile);
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
    if (settings.value("menubarenabled", false).toBool())
        ui->menuBar->show();
    else
        ui->menuBar->hide();

    //slideshowtimer
    slideshowTimer->setInterval(settings.value("slideshowtimer", 5).toInt()*1000);

    //window resize mode
    windowResizeMode = settings.value("windowresizemode", 1).toInt();

    //max window resize mode size
    maxWindowResizedPercentage = settings.value("maxwindowresizedpercentage", 70).toReal()/100;

    ui->graphicsView->loadSettings();

    loadShortcuts();
}

void MainWindow::loadShortcuts() {
    QSettings settings;
    settings.beginGroup("shortcuts");

    typedef QVOptionsDialog qvo;

    // To retrieve default bindings, we hackily init an options dialog and use it's constructor values
    qvo invisibleOptionsDialog;
    auto shortcutData = invisibleOptionsDialog.getTransientShortcuts();

    // Iterate through all default shortcuts to get saved shortcuts from settings
    QHash<QString, QList<QKeySequence>> shortcuts;
    QListIterator<QVShortcutDialog::SShortcut> iter(shortcutData);
    while (iter.hasNext())
    {
        auto value = iter.next();
        shortcuts.insert(value.name, qvo::stringListToKeySequenceList(settings.value(value.name, value.defaultShortcuts).value<QStringList>()));
    }

    // Set shortcuts by name from above list
    ui->actionOpen->setShortcuts(shortcuts.value("open"));
    ui->actionFirst_File->setShortcuts(shortcuts.value("firstfile"));
    ui->actionPrevious_File->setShortcuts(shortcuts.value("previousfile"));
    ui->actionNext_File->setShortcuts(shortcuts.value("nextfile"));
    ui->actionLast_File->setShortcuts(shortcuts.value("lastfile"));
    ui->actionCopy->setShortcuts(shortcuts.value("copy"));
    ui->actionPaste->setShortcuts(shortcuts.value("paste"));
    ui->actionRotate_Right->setShortcuts(shortcuts.value("rotateright"));
    ui->actionRotate_Left->setShortcuts(shortcuts.value("rotateleft"));
    ui->actionZoom_In->setShortcuts(shortcuts.value("zoomin"));
    ui->actionZoom_Out->setShortcuts(shortcuts.value("zoomout"));
    ui->actionReset_Zoom->setShortcuts(shortcuts.value("resetzoom"));
    ui->actionMirror->setShortcuts(shortcuts.value("mirror"));
    ui->actionFlip->setShortcuts(shortcuts.value("flip"));
    ui->actionFull_Screen->setShortcuts(shortcuts.value("fullscreen"));
    ui->actionOriginal_Size->setShortcuts(shortcuts.value("originalsize"));
    ui->actionNew_Window->setShortcuts(shortcuts.value("newwindow"));
    ui->actionNext_Frame->setShortcuts(shortcuts.value("nextframe"));
    ui->actionPause->setShortcuts(shortcuts.value("pause"));
    ui->actionIncrease_Speed->setShortcuts(shortcuts.value("increasespeed"));
    ui->actionDecrease_Speed->setShortcuts(shortcuts.value("decreasespeed"));
    ui->actionReset_Speed->setShortcuts(shortcuts.value("resetspeed"));
    ui->actionProperties->setShortcuts(shortcuts.value("showfileinfo"));
    ui->actionOptions->setShortcuts(shortcuts.value("options"));
    ui->actionQuit->setShortcuts(shortcuts.value("quit"));
}

void MainWindow::updateRecentMenu()
{
    QSettings settings;
    settings.beginGroup("recents");

    //get recent files from config file
    QVariantList recentFiles = settings.value("recentFiles").value<QVariantList>();

    //on macOS, we have to clear the menu, re-add items, and add the old items again because it displays hidden qactions as grayed out
    #ifdef Q_OS_MACX
    dockMenu->clear();
    #endif
    for (int i = 0; i <= 9; i++)
    {
        if (i < recentFiles.size())
        {
            recentItems[i]->setVisible(true);
            recentItems[i]->setText(recentFiles[i].toList().first().toString());

            //re-add items after clearing dock menu
            #ifdef Q_OS_MACX
            dockMenu->addAction(recentItems[i]);
            #endif
            //set menu icons for linux users
            QMimeDatabase mimedb;
            QMimeType type = mimedb.mimeTypeForFile(recentFiles[i].toList().last().toString());
            if (type.iconName().isNull())
                recentItems[i]->setIcon(QIcon::fromTheme(type.genericIconName()));
            else
                  recentItems[i]->setIcon(QIcon::fromTheme(type.iconName()));
        }
        else
        {
            recentItems[i]->setVisible(false);
            recentItems[i]->setText(tr("Empty"));
        }
    }
    //re-add original items to dock menu after adding recents
    #ifdef Q_OS_MACX
    dockMenu->addAction(ui->actionNew_Window);
    dockMenu->addAction(ui->actionOpen);
    dockMenu->insertSeparator(ui->actionNew_Window);
    #endif
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

    updateRecentMenu();
}

void MainWindow::cancelSlideshow()
{
    if (slideshowTimer->isActive())
        on_actionSlideshow_triggered();
}

void MainWindow::fileLoaded()
{
    //activate items after item is loaded for the first time
    if (ui->graphicsView->getCurrentFileDetails().isPixmapLoaded && !ui->actionOpen_Containing_Folder->isEnabled())
    {
        foreach(QAction* action, ui->menuView->actions())
            action->setEnabled(true);
        foreach(QAction* action, contextMenu->actions())
            action->setEnabled(true);
        foreach(QAction* action, actions())
            action->setEnabled(true);
        ui->actionSlideshow->setEnabled(true);
        ui->actionCopy->setEnabled(true);
    }
    //disable gif controls if there is no gif loaded
    ui->menuTools->actions().constFirst()->setEnabled(ui->graphicsView->getCurrentFileDetails().isMovieLoaded);
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

// Actions

void MainWindow::on_actionOpen_triggered()
{
    pickFile();
}

void MainWindow::on_actionAbout_triggered()
{
    auto *about = new QVAboutDialog(this);
    about->exec();
}

void MainWindow::on_actionCopy_triggered()
{
    auto mimeData = ui->graphicsView->getMimeData();
    if (!mimeData->hasImage() || !mimeData->hasUrls())
        return;

    QApplication::clipboard()->setMimeData(mimeData);
}

void MainWindow::on_actionPaste_triggered()
{
    ui->graphicsView->loadMimeData(QApplication::clipboard()->mimeData());
}

void MainWindow::on_actionOptions_triggered()
{
    auto *options = new QVOptionsDialog(this);
    options->open();

    connect(options, &QVOptionsDialog::optionsSaved, this, &MainWindow::loadSettings);
}

void MainWindow::on_actionNext_File_triggered()
{
    ui->graphicsView->goToFile(QVGraphicsView::goToFileMode::next);
}

void MainWindow::on_actionPrevious_File_triggered()
{
    ui->graphicsView->goToFile(QVGraphicsView::goToFileMode::previous);
}

void MainWindow::on_actionOpen_Containing_Folder_triggered()
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

void MainWindow::on_actionRotate_Right_triggered()
{
    ui->graphicsView->rotateImage(90);
    ui->graphicsView->resetScale();
}

void MainWindow::on_actionRotate_Left_triggered()
{
    ui->graphicsView->rotateImage(-90);
    ui->graphicsView->resetScale();
}

void MainWindow::on_actionWelcome_triggered()
{
    auto *welcome = new QVWelcomeDialog(this);
    welcome->exec();
}

void MainWindow::on_actionMirror_triggered()
{
    ui->graphicsView->scale(-1, 1);
    ui->graphicsView->resetScale();
}

void MainWindow::on_actionFlip_triggered()
{
    ui->graphicsView->scale(1, -1);
    ui->graphicsView->resetScale();
}

void MainWindow::on_actionZoom_In_triggered()
{
    ui->graphicsView->zoom(120, ui->graphicsView->mapFromGlobal(QCursor::pos()));
}

void MainWindow::on_actionZoom_Out_triggered()
{
    ui->graphicsView->zoom(-120, ui->graphicsView->mapFromGlobal(QCursor::pos()));
}

void MainWindow::on_actionReset_Zoom_triggered()
{
    ui->graphicsView->resetScale();
}

void MainWindow::on_actionProperties_triggered()
{
    refreshProperties();
    info->show();
}

void MainWindow::on_actionFull_Screen_triggered()
{
    if (windowState() == Qt::WindowFullScreen)
        showNormal();
    else
        showFullScreen();
}

void MainWindow::on_actionOriginal_Size_triggered()
{
    ui->graphicsView->originalSize();
}

void MainWindow::on_actionNew_Window_triggered()
{
    QVApplication::newWindow();
}

void MainWindow::on_actionSlideshow_triggered()
{
    if (slideshowTimer->isActive())
    {
        slideshowTimer->stop();
        ui->actionSlideshow->setText(tr("Start Slideshow"));
        ui->actionSlideshow->setIcon(QIcon::fromTheme("media-playback-start"));
    }
    else
    {
        slideshowTimer->start();
        ui->actionSlideshow->setText(tr("Stop Slideshow"));
        ui->actionSlideshow->setIcon(QIcon::fromTheme("media-playback-stop"));
    }
}

void MainWindow::slideshowAction()
{
    QSettings settings;
    settings.beginGroup("options");
    if(settings.value("slideshowdirection", 0).toInt() == 0)
        on_actionNext_File_triggered();
    else
        on_actionPrevious_File_triggered();
}

void MainWindow::on_actionPause_triggered()
{
    if (!ui->graphicsView->getCurrentFileDetails().isMovieLoaded)
        return;

    if (ui->graphicsView->getLoadedMovie().state() == QMovie::Running)
    {
        ui->graphicsView->setPaused(true);
        ui->actionPause->setText(tr("Resume"));
        ui->actionPause->setIcon(QIcon::fromTheme("media-playback-start"));
    }
    else
    {
        ui->graphicsView->setPaused(false);
        ui->actionPause->setText(tr("Pause"));
        ui->actionPause->setIcon(QIcon::fromTheme("media-playback-pause"));
    }
}

void MainWindow::on_actionNext_Frame_triggered()
{
    if (!ui->graphicsView->getCurrentFileDetails().isMovieLoaded)
        return;

    ui->graphicsView->jumpToNextFrame();
}

void MainWindow::on_actionReset_Speed_triggered()
{
    if (!ui->graphicsView->getCurrentFileDetails().isMovieLoaded)
        return;

    ui->graphicsView->setSpeed(100);
}

void MainWindow::on_actionDecrease_Speed_triggered()
{
    if (!ui->graphicsView->getCurrentFileDetails().isMovieLoaded)
        return;

    ui->graphicsView->setSpeed(ui->graphicsView->getLoadedMovie().speed()-25);
}

void MainWindow::on_actionIncrease_Speed_triggered()
{
    if (!ui->graphicsView->getCurrentFileDetails().isMovieLoaded)
        return;

    ui->graphicsView->setSpeed(ui->graphicsView->getLoadedMovie().speed()+25);
}

void MainWindow::on_actionSave_Frame_As_triggered()
{
    QSettings settings;
    settings.beginGroup("recents");
    if (!ui->graphicsView->getCurrentFileDetails().isMovieLoaded)
        return;

    ui->graphicsView->setPaused(true);
    ui->actionPause->setText(tr("Resume"));
    QFileDialog *saveDialog = new QFileDialog(this, tr("Save Frame As..."), "", tr("Supported Files (*.bmp *.cur *.icns *.ico *.jp2 *.jpeg *.jpe *.jpg *.pbm *.pgm *.png *.ppm *.tif *.tiff *.wbmp *.webp *.xbm *.xpm);;All Files (*)"));
    saveDialog->setDirectory(settings.value("lastFileDialogDir", QDir::homePath()).toString());
    saveDialog->selectFile(ui->graphicsView->getCurrentFileDetails().fileInfo.baseName() + "-" + QString::number(ui->graphicsView->getLoadedMovie().currentFrameNumber()) + ".png");
    saveDialog->setDefaultSuffix("png");
    saveDialog->setAcceptMode(QFileDialog::AcceptSave);
    saveDialog->open();
    connect(saveDialog, &QFileDialog::fileSelected, this, [=](QString fileName){
        ui->graphicsView->originalSize();
        for(int i=0; i<=ui->graphicsView->getLoadedMovie().frameCount(); i++)
            ui->graphicsView->jumpToNextFrame();
        on_actionNext_Frame_triggered();
        ui->graphicsView->getLoadedMovie().currentPixmap().save(fileName, nullptr, 100);
        ui->graphicsView->resetScale();
    });
}

void MainWindow::on_actionQuit_triggered()
{
    close();
    QCoreApplication::quit();
}

void MainWindow::on_actionFirst_File_triggered()
{
    ui->graphicsView->goToFile(QVGraphicsView::goToFileMode::first);
}

void MainWindow::on_actionLast_File_triggered()
{
    ui->graphicsView->goToFile(QVGraphicsView::goToFileMode::last);
}

void MainWindow::on_actionOpen_URL_triggered()
{
    auto inputDialog = new QInputDialog(this);
    inputDialog->setWindowTitle(ui->actionOpen_URL->text());
    inputDialog->setLabelText(tr("URL of a supported image file:"));
    inputDialog->resize(350, inputDialog->height());
    connect(inputDialog, &QInputDialog::finished, [inputDialog, this](int result) {
        if (!result)
            return;

        auto url = QUrl(inputDialog->textValue());
        inputDialog->deleteLater();

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
        progressDialog->setWindowTitle(ui->actionOpen_URL->text());
        progressDialog->open();

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
                if(saveFutureWatcher->result())
                {
                    if(tempFile->open())
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
    });
    inputDialog->open();
}
