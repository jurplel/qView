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

    //hide menubar for non-global applications
    ui->menuBar->hide();

    //change show in explorer text based on operating system

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

    //Context menu
    menu = new QMenu(this);

    menu->addAction(ui->actionOpen);

    QMenu *files = new QMenu("Open Recent");

    int index = 0;

    for ( int i = 0; i <= 9; i++ )
    {
        recentItems.append(new QAction("error", this));
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

    menu->addAction(ui->actionNext_File);
    menu->addAction(ui->actionPrevious_File);
    menu->addAction(ui->actionOpen_Containing_Folder);
    menu->addAction(ui->actionPaste);
    menu->addSeparator();

    QMenu *zoom = new QMenu("Zoom", this);
    zoom->addAction(ui->actionZoom_In);
    zoom->addAction(ui->actionZoom_Out);
    zoom->addAction(ui->actionReset_Zoom);
    menu->addMenu(zoom);

    QMenu *rotate = new QMenu("Rotate", this);
    rotate->addAction(ui->actionRotate_Right);
    rotate->addAction(ui->actionRotate_Left);
    menu->addMenu(rotate);

    QMenu *flip = new QMenu("Flip", this);
    flip->addAction(ui->actionFlip_Horizontally);
    flip->addAction(ui->actionFlip_Vertically);
    menu->addMenu(flip);

    menu->addSeparator();
    menu->addAction(ui->actionOptions);

    QMenu *help = new QMenu("Help", this);
    help->addAction(ui->actionAbout);
    help->addAction(ui->actionWelcome);
    menu->addMenu(help);

    //add recent items to other places
    ui->menuFile->insertMenu(ui->actionNext_File, files);
    files->setAsDockMenu();

    updateMenus();
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
    QFileDialog *fileDialog = new QFileDialog(this, tr("Open"), "", tr("Images (*.svg *.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm);;All Files (*)"));
    fileDialog->open();
    connect(fileDialog, &QFileDialog::fileSelected, this, &MainWindow::openFile);
}

void MainWindow::openFile(QString fileName)
{
    ui->graphicsView->loadFile(fileName);
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
}

void MainWindow::saveGeometrySettings()
{
    settings.setValue("geometry", saveGeometry());
}

void MainWindow::updateMenus()
{
    int index = 0;
    foreach(QAction* action, recentItems)
    {
        QString newFileName = "filename" + QString::number(index);
        QString newText = settings.value(newFileName).toString();
        if (!newText.isEmpty())
        {
            action->setVisible(true);
            action->setText(newText);
        }
        else
        {
            action->setVisible(false);
        }
        index++;
    }
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

void MainWindow::openRecent(int i)
{
    ui->graphicsView->loadFile(settings.value("file" + QString::number(i)).toString());
}

void MainWindow::clearRecent()
{
    QSettings settings;
    for (int i = 0; i < 9; i++)
    {
        settings.remove("filename" + QString::number(i));
    }
    settings.setValue("file0", ui->graphicsView->getSelectedFileInfo().filePath());
    settings.setValue("filename0", ui->graphicsView->getSelectedFileInfo().fileName());
    updateMenus();
}
