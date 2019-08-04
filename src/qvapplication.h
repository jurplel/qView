#ifndef QVAPPLICATION_H
#define QVAPPLICATION_H

#include <QApplication>
#include "mainwindow.h"

class QVApplication : public QApplication
{
    Q_OBJECT

public:
    explicit QVApplication(int &argc, char **argv);
    ~QVApplication() override;

    bool event(QEvent *event) override;

    static void pickFile();

    static void openFile(const QString &file, bool resize = true);

    static MainWindow *newWindow();

    static MainWindow *getMainWindow();

    void updateDockRecents();

private:
    QMenu *dockMenu;
};

#endif // QVAPPLICATION_H
