#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QPixmap>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_actionOpen_triggered();

    void on_actionAbout_Qt_triggered();

    void on_actionAbout_triggered();

private:
    Ui::MainWindow *ui;

    void pickFile();

    QGraphicsScene *scene;
    QPixmap loadedPixmap;

    bool isPixmapLoaded;

};

#endif // MAINWINDOW_H
