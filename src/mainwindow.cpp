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
#include <QDebug>
#include <QPixmap>
#include <QClipboard>
#include <QCoreApplication>
#include <QFileSystemWatcher>
#include <QProcess>
#include <QDesktopServices>
#include <QContextMenuEvent>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //load settings from file
    loadSettings();

    //enable drag&dropping
    setAcceptDrops(true);

    //make info dialog
    info = new QVInfoDialog(this);

    //change show in file explorer text based on operating system
    #if defined(Q_OS_WIN)
    ui->actionOpen_Containing_Folder->setText("Show in Explorer");
    #elif defined(Q_OS_MACX)
    ui->actionOpen_Containing_Folder->setText("Show in Finder");
    #endif

    //Keyboard Shortcuts
    ui->actionOpen->setShortcut(QKeySequence::Open);
    ui->actionNext_File->setShortcut(Qt::Key_Right);
    ui->actionPrevious_File->setShortcut(Qt::Key_Left);
    ui->actionPaste->setShortcut(QKeySequence::Paste);
    ui->actionRotate_Right->setShortcut(Qt::Key_Up);
    ui->actionRotate_Left->setShortcut(Qt::Key_Down);
    ui->actionZoom_In->setShortcuts(QList<QKeySequence>({QKeySequence(Qt::CTRL + Qt::Key_Equal), QKeySequence::ZoomIn}));
    ui->actionZoom_Out->setShortcut(QKeySequence::ZoomOut);
    ui->actionReset_Zoom->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
    ui->actionFull_Screen->setShortcut(QKeySequence::FullScreen);

    //Context menu
    menu = new QMenu(this);

    menu->addAction(ui->actionOpen);

    QMenu *files = new QMenu("Open Recent");

    int index = 0;

    for ( int i = 0; i <= 9; i++ )
    {
        recentItems.append(new QAction("Empty", this));
    }

    foreach(QAction *action, recentItems)
    {
        connect(action, &QAction::triggered, [this,index]() { openRecent(index); });
        files->addAction(action);
        index++;
    }
    files->addSeparator();
    QAction *clearMenu = new QAction("Clear Menu", this);
    connect(clearMenu, &QAction::triggered, this, &MainWindow::clearRecent);
    files->addAction(clearMenu);

    menu->addMenu(files);

    menu->addAction(ui->actionOpen_Containing_Folder);
    menu->addAction(ui->actionProperties);
    menu->addAction(ui->actionPaste);
    menu->addSeparator();
    menu->addAction(ui->actionNext_File);
    menu->addAction(ui->actionPrevious_File);
    menu->addSeparator();

    QMenu *zoom = new QMenu("Zoom", this);
    zoom->menuAction()->setEnabled(false);
    zoom->addAction(ui->actionZoom_In);
    zoom->addAction(ui->actionZoom_Out);
    zoom->addAction(ui->actionReset_Zoom);
    menu->addMenu(zoom);

    QMenu *rotate = new QMenu("Rotate", this);
    rotate->menuAction()->setEnabled(false);
    rotate->addAction(ui->actionRotate_Right);
    rotate->addAction(ui->actionRotate_Left);
    menu->addMenu(rotate);

    QMenu *flip = new QMenu("Flip", this);
    flip->menuAction()->setEnabled(false);
    flip->addAction(ui->actionFlip_Horizontally);
    flip->addAction(ui->actionFlip_Vertically);
    menu->addMenu(flip);

    menu->addAction(ui->actionFull_Screen);
    menu->addSeparator();
    menu->addAction(ui->actionOptions);

    QMenu *help = new QMenu("Help", this);
    help->addAction(ui->actionAbout);
    help->addAction(ui->actionWelcome);
    menu->addMenu(help);

    //add recent items to menubar
    ui->menuFile->insertMenu(ui->actionOpen_Containing_Folder, files);

    #ifdef Q_OS_MACX
    //macOS dock menu
    dockMenu = new QMenu(this);
    dockMenu->setAsDockMenu();
    dockMenu->addAction(ui->actionOpen);
    #endif

    //add to mainwindow's action list so keyboard shortcuts work without a menubar
    foreach(QAction *action, menu->actions())
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
    menu->exec(event->globalPos());
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    if (settings.value("firstlaunch", false).toBool())
        return;

    settings.setValue("firstlaunch", true);
    QTimer::singleShot(100, this, &MainWindow::on_actionWelcome_triggered);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveGeometrySettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::pickFile()
{
    QFileDialog *fileDialog = new QFileDialog(this, tr("Open"), "", tr("Supported Files (*.bmp *.cur *.gif *.icns *.ico *.jpeg *.jpg *.pbm *.pgm *.png *.ppm *.svg *.svgz *.tif *.tiff *.wbmp *.webp *.xbm *.xpm);;All Files (*)"));
    fileDialog->setDirectory(settings.value("lastFileDialogDir", QDir::homePath()).toString());
    fileDialog->open();
    connect(fileDialog, &QFileDialog::fileSelected, this, &MainWindow::openFile);
}

void MainWindow::openFile(QString fileName)
{
    ui->graphicsView->loadFile(fileName);
    settings.setValue("lastFileDialogDir", QFileInfo(fileName).path());
}


void MainWindow::loadSettings()
{

    //geometry
    restoreGeometry(settings.value("geometry").toByteArray());

    //bgcolor
    QBrush newBrush;
    newBrush.setStyle(Qt::SolidPattern);
    if (!((settings.value("bgcolorenabled", true).toBool())))
    {
        newBrush.setColor(QColor("#00000000"));
    }
    else
    {
        QColor newColor;
        newColor.setNamedColor(settings.value("bgcolor", QString("#212121")).toString());
        newBrush.setColor(newColor);
    }
    ui->graphicsView->setBackgroundBrush(newBrush);

    //filtering
    ui->graphicsView->setIsFilteringEnabled(settings.value("filteringenabled", true).toBool());

    //scaling
    ui->graphicsView->setIsScalingEnabled(settings.value("scalingenabled", true).toBool());

    //titlebar
    ui->graphicsView->setTitlebarMode(settings.value("titlebarmode", 1).toInt());

    //menubar
    if (settings.value("menubarenabled", false).toBool())
    {
        ui->menuBar->show();
    }
    else
    {
        ui->menuBar->hide();
    }
}

void MainWindow::saveGeometrySettings()
{
    settings.setValue("geometry", saveGeometry());
}

void MainWindow::updateRecentMenu()
{
    if (ui->graphicsView->getIsPixmapLoaded() && !ui->actionOpen_Containing_Folder->isEnabled())
    {
        foreach(QAction* action, ui->menuView->actions())
        {
            action->setEnabled(true);
        }
        foreach(QAction* action, menu->actions())
        {
            action->setEnabled(true);
        }
    }

    QVariantList recentFiles = settings.value("recentFiles").value<QVariantList>();

    #ifdef Q_OS_MACX
    dockMenu->clear();
    #endif
    for (int i = 0; i <= 9; i++)
    {
        if (i < recentFiles.size())
        {
            recentItems[i]->setVisible(true);
            recentItems[i]->setText(recentFiles[i].toList().first().toString());
            #ifdef Q_OS_MACX
            dockMenu->addAction(recentItems[i]);
            #endif
        }
        else
        {
            recentItems[i]->setVisible(false);
            recentItems[i]->setText("Empty");
        }
    }
    #ifdef Q_OS_MACX
    dockMenu->addAction(ui->actionOpen);
    dockMenu->insertSeparator(ui->actionOpen);
    #endif
}

void MainWindow::openRecent(int i)
{
    QVariantList recentFiles = settings.value("recentFiles").value<QVariantList>();
    ui->graphicsView->loadFile(recentFiles[i].toList().last().toString());
}

void MainWindow::clearRecent()
{
    QVariantList recentFiles = settings.value("recentFiles").value<QVariantList>();
    for (int i = 0; i <= 9; i++)
    {
        if (recentFiles.size() > 1)
        {
            recentFiles.removeAt(1);
        }
        else
        {
            if (!ui->graphicsView->getIsPixmapLoaded())
                recentFiles.removeAt(0);
        }
    }
    settings.setValue("recentFiles", recentFiles);

    updateRecentMenu();
}

void MainWindow::refreshProperties()
{
    info->setInfo(ui->graphicsView->getSelectedFileInfo(), ui->graphicsView->getImageWidth(), ui->graphicsView->getImageHeight());
}

// Actions

void MainWindow::on_actionOpen_triggered()
{
    pickFile();
}

void MainWindow::on_actionAbout_triggered()
{
    QVAboutDialog *about = new QVAboutDialog(this);
    about->exec();
}

void MainWindow::on_actionPaste_triggered()
{
    ui->graphicsView->loadMimeData(QApplication::clipboard()->mimeData());
}

void MainWindow::on_actionOptions_triggered()
{
    QVOptionsDialog *options = new QVOptionsDialog(this);
    options->open();

    connect(options, &QVOptionsDialog::optionsSaved, this, &MainWindow::saveGeometrySettings);
    connect(options, &QVOptionsDialog::optionsSaved, this, &MainWindow::loadSettings);
}

void MainWindow::on_actionNext_File_triggered()
{
    ui->graphicsView->nextFile();
}

void MainWindow::on_actionPrevious_File_triggered()
{
    ui->graphicsView->previousFile();
}

void MainWindow::on_actionOpen_Containing_Folder_triggered()
{
    if (!ui->graphicsView->getIsPixmapLoaded())
        return;

    const QFileInfo selectedFileInfo = ui->graphicsView->getSelectedFileInfo();

    QProcess process;

    #if defined(Q_OS_WIN)
    process.startDetached("explorer", QStringList() << "/select," << QDir::toNativeSeparators(selectedFileInfo.absoluteFilePath()));
    #elif defined(Q_OS_MACX)
    process.execute("open", QStringList() << "-R" << selectedFileInfo.absoluteFilePath());
    #else
    QDesktopServices::openUrl(selectedFileInfo.absolutePath());
    #endif

    return;
}

void MainWindow::on_actionRotate_Right_triggered()
{
    ui->graphicsView->rotate(90);
    ui->graphicsView->resetScale();
}

void MainWindow::on_actionRotate_Left_triggered()
{
    ui->graphicsView->rotate(-90);
    ui->graphicsView->resetScale();
}

void MainWindow::on_actionWelcome_triggered()
{
    QVWelcomeDialog *welcome = new QVWelcomeDialog(this);
    welcome->exec();
}

void MainWindow::on_actionFlip_Horizontally_triggered()
{
    ui->graphicsView->scale(-1, 1);
}

void MainWindow::on_actionFlip_Vertically_triggered()
{
    ui->graphicsView->scale(1, -1);
}

void MainWindow::on_actionZoom_In_triggered()
{
    ui->graphicsView->zoom(120);
}

void MainWindow::on_actionZoom_Out_triggered()
{
    ui->graphicsView->zoom(-120);
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
    {
        showNormal();
    }
    else
    {
        showFullScreen();
    }
}
