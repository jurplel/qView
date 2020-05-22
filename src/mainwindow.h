#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qvinfodialog.h"
#include "qvimagecore.h"
#include "qvgraphicsview.h"

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

    void buildWindowTitle();

    void setWindowSize();

    bool getIsPixmapLoaded() const;

    void setJustLaunchedWithImage(bool value);

    QScreen *screenAt(const QPoint &point);

    void openRecent(int i);

    void openUrl(const QUrl &url);

    void pickUrl();

    void openContainingFolder();

    void showFileInfo();

    void deleteFile();

    void copy();

    void paste();

    void rename();

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

    void saveFrameAs();

    void pause();

    void nextFrame();

    void decreaseSpeed();

    void resetSpeed();

    void increaseSpeed();

    void toggleFullScreen();

    const QVImageCore::QVFileDetails& getCurrentFileDetails() const { return graphicsView->getCurrentFileDetails(); }

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

    void changeEvent(QEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;

protected slots:
    void settingsUpdated();
    void shortcutsUpdated();

private:
    Ui::MainWindow *ui;
    QVGraphicsView *graphicsView;

    QMenu *contextMenu;
    QMenu *virtualMenu;

    QTimer *slideshowTimer;

    QShortcut *escShortcut;

    QVInfoDialog *info;

    bool justLaunchedWithImage;
};

#endif // MAINWINDOW_H
