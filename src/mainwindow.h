#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qvinfodialog.h"
#include <QMainWindow>
#include <QShortcut>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void refreshProperties();

    void setWindowSize();

    bool getIsPixmapLoaded() const;

    void setJustLaunchedWithImage(bool value);

    QScreen *screenAt(const QPoint &point);

    void pickFile();

    void openRecent(int i);

    void openUrl(const QUrl &url);

    void pickUrl();

    void openContainingFolder();

    void showFileInfo();

    void copy();

    void paste();

    void zoomIn();

    void zoomOut();

    void resetZoom();

    void originalSize();

    void rotateRight();

    void rotateLeft();

    void mirror();

    void flip();

    void firstFile();

    void previousFile();

    void nextFile();

    void lastFile();

    void openOptions();

    void openAbout();

    void openWelcome();

    void saveFrameAs();

    void pause();

    void nextFrame();

    void decreaseSpeed();

    void resetSpeed();

    void increaseSpeed();

    void toggleFullScreen();

    void quit();

public slots:
    void openFile(const QString &fileName);

    void toggleSlideshow();

    void slideshowAction();

    void cancelSlideshow();

    void fileLoaded();

protected:
    bool event(QEvent *event) override;

    void contextMenuEvent(QContextMenuEvent *event) override;

    void showEvent(QShowEvent *event) override;

    void closeEvent(QCloseEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void loadSettings();

private:
    Ui::MainWindow *ui;

    QMenu *contextMenu;
    QMenu *virtualMenu;

    QTimer *slideshowTimer;

    QShortcut *escShortcut;

    QVInfoDialog *info;

    bool menuBarEnabled;
    bool slideshowReversed;
    int windowResizeMode;
    bool justLaunchedWithImage;
    qreal maxWindowResizedPercentage;
};

#endif // MAINWINDOW_H
