#ifndef QVAPPLICATION_H
#define QVAPPLICATION_H

#include <QApplication>
#include <QMainWindow>
#include "mainwindow.h"

class QVApplication : public QApplication
{
public:
    QVApplication(int &argc, char **argv);

    bool event(QEvent *event);

    MainWindow* getMainWindow();
};

#endif // QVAPPLICATION_H
