#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qvoptionsdialog.h"
#include "qvaboutdialog.h"
#include "qvapplication.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QGraphicsPixmapItem>
#include <QDebug>
#include <QPixmap>
#include <QGraphicsScene>
#include <QClipboard>
#include <QCoreApplication>
#include <QFileSystemWatcher>
#include <QProcess>
#include <QDesktopServices>

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

    //context menu items
    ui->graphicsView->addAction(ui->actionOpen);
    ui->graphicsView->addAction(ui->actionNext_File);
    ui->graphicsView->addAction(ui->actionPrevious_File);
    ui->graphicsView->addAction(ui->actionOpen_Containing_Folder);
    ui->graphicsView->addAction(ui->actionPaste);
    ui->graphicsView->addAction(ui->actionOptions);
    ui->graphicsView->addAction(ui->actionAbout);

    //qgraphicsscene setup
    scene = new QGraphicsScene(0.0, 0.0, 100000.0, 100000.0, this);
    ui->graphicsView->setScene(scene);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveGeometrySettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::pickFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open"), "", tr("Images (*.svg *.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm);;All Files (*)"));
    openFile(fileName);
}

void MainWindow::openFile(QString fileName)
{
    ui->graphicsView->loadFile(fileName);
}


void MainWindow::loadSettings()
{
    QSettings settings;

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
}

void MainWindow::saveGeometrySettings()
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
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
    connect(options, SIGNAL(optionsSaved()), this, SLOT(saveGeometrySettings()));
    connect(options, SIGNAL(optionsSaved()), this, SLOT(loadSettings()));
}

void MainWindow::on_actionPrevious_File_triggered()
{
    ui->graphicsView->previousFile();
}

void MainWindow::on_actionNext_File_triggered()
{
    ui->graphicsView->nextFile();
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
