#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qvoptionsdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QGraphicsPixmapItem>
#include <QDebug>
#include <QPixmap>
#include <QGraphicsScene>
#include <QClipboard>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //enable drag&dropping
    setAcceptDrops(true);

    //hide menubar for non-global applications
    ui->menuBar->hide();

    //Keyboard Shortcuts
    ui->actionOpen->setShortcut(Qt::Key_O | Qt::CTRL);
    ui->actionPaste->setShortcut(Qt::Key_V | Qt::CTRL);

    //context menu items
    ui->graphicsView->addAction(ui->actionOpen);
    ui->graphicsView->addAction(ui->actionPaste);
    ui->graphicsView->addAction(ui->actionOptions);
    ui->graphicsView->addAction(ui->actionAbout);
    ui->graphicsView->addAction(ui->actionAbout_Qt);

    //qgraphicsscene setup
    scene = new QGraphicsScene(0.0, 0.0, 100000.0, 100000.0, this);
    ui->graphicsView->setScene(scene);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::pickFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open"), "",
        tr("Images (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm);;All Files (*)"));
    openFile(fileName);
}

void MainWindow::openFile(QString fileName)
{
    ui->graphicsView->loadFile(fileName);
}

// Actions

void MainWindow::on_actionOpen_triggered()
{
    pickFile();
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(ui->centralWidget, QString("About Qt"));
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(ui->centralWidget, QString("About qView"), QString("qView pre-release %1 by jurplel").arg(VERSION));
}

void MainWindow::on_actionPaste_triggered()
{
    ui->graphicsView->loadMimeData(QApplication::clipboard()->mimeData());
}

void MainWindow::on_actionOptions_triggered()
{
    QVOptionsDialog *options = new QVOptionsDialog(this);
    options->show();
}
