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

    void openFile(QString fileName);

    bool getIsPixmapLoaded() const;
    void setIsPixmapLoaded(bool value);

private slots:
    void loadSettings();

    void on_actionOpen_triggered();

    void on_actionAbout_Qt_triggered();

    void on_actionAbout_triggered();

    void on_actionPaste_triggered();

    void on_actionOptions_triggered();

    void on_actionPrevious_File_triggered();

    void on_actionNext_File_triggered();

private:
    Ui::MainWindow *ui;

    void pickFile();

    QGraphicsScene *scene;
    QPixmap loadedPixmap;
};

#endif // MAINWINDOW_H
