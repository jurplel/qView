#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::PickFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open"), "",
        tr("All Files (*)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
}

void MainWindow::on_actionOpen_triggered()
{
    PickFile();
}
