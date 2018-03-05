#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QGraphicsPixmapItem>
#include <QDebug>
#include <QPixmap>
#include <QGraphicsScene>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //default variable values
    isPixmapLoaded = false;

    //context menu items
    ui->graphicsView->addAction(ui->actionOpen);
    ui->graphicsView->addAction(ui->actionAbout);
    ui->graphicsView->addAction(ui->actionAbout_Qt);

    //graphicsview setup
    scene = new QGraphicsScene(0.0, 0.0, 100000.0, 100000.0, this);
    ui->graphicsView->setScene(scene);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if(isPixmapLoaded)
    {
        ui->graphicsView->resetScale(loadedPixmapItem);
    }
}

void MainWindow::pickFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open"), "",
        tr("Images (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    if(!loadedPixmap.load(fileName))
        return;

    if(loadedPixmap.isNull())
        return;
    scene->clear();
    loadedPixmapItem = scene->addPixmap(loadedPixmap);
    loadedPixmapItem->setOffset((50000.0 - loadedPixmap.width()/2), (50000.0 - loadedPixmap.height()/2));
    ui->graphicsView->resetScale(loadedPixmapItem);
    loadedPixmapItem->setTransformationMode(Qt::SmoothTransformation);
    isPixmapLoaded = true;
}
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
    QMessageBox::about(ui->centralWidget, QString("About"), QString("unlabeled qView alpha by jurplel"));
}
