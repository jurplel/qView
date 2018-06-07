#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qvinfodialog.h"
#include <QMainWindow>
#include <QPixmap>
#include <QSettings>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void updateRecentMenu();

    void refreshProperties();

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

    void on_actionZoom_In_triggered();

    void on_actionZoom_Out_triggered();

    void on_actionReset_Zoom_triggered();

    void openRecent(int i);

    void clearRecent();

    void on_actionProperties_triggered();

    void on_actionFull_Screen_triggered();

private:
    Ui::MainWindow *ui;

    void pickFile();

    QSettings settings;
    QMenu *menu;
    QMenu *dockMenu;

    QList<QAction*> recentItems;

    QVInfoDialog *info;
};

#endif // MAINWINDOW_H
