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

    bool getIsPixmapLoaded() const;
    void setIsPixmapLoaded(bool value);

public slots:
    void openFile(QString fileName);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

    void showEvent(QShowEvent *event) override;

    void closeEvent(QCloseEvent *event) override;

private slots:

    void loadSettings();

    void saveGeometrySettings();

    void on_actionOpen_triggered();

    void on_actionAbout_triggered();

    void on_actionPaste_triggered();

    void on_actionOptions_triggered();

    void on_actionPrevious_File_triggered();

    void on_actionNext_File_triggered();

    void on_actionOpen_Containing_Folder_triggered();

    void on_actionWelcome_triggered();

    void on_actionRotate_Right_triggered();

    void on_actionRotate_Left_triggered();

    void on_actionFlip_Horizontally_triggered();

    void on_actionFlip_Vertically_triggered();

private:
    Ui::MainWindow *ui;

    void pickFile();

    QGraphicsScene *scene;
    QPixmap loadedPixmap;
};

#endif // MAINWINDOW_H
