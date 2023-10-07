#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qvinfodialog.h"
#include "qvimagecore.h"
#include "qvgraphicsview.h"
#include "openwith.h"

#include <QMainWindow>
#include <QShortcut>
#include <QNetworkAccessManager>
#include <QStack>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    struct DeletedPaths
    {
        QString pathInTrash;
        QString previousPath;
    };

    explicit MainWindow(QWidget *parent = nullptr, const QJsonObject &windowSessionState = {});
    ~MainWindow() override;

    void requestPopulateOpenWithMenu();

    void populateOpenWithMenu(const QList<OpenWith::OpenWithItem> openWithItems);

    void refreshProperties();

    void buildWindowTitle();

    void updateWindowFilePath();

    void updateMenuBarVisible();

    bool getTitlebarHidden() const;

    void setTitlebarHidden(const bool shouldHide);

    void setWindowSize();

    bool getIsPixmapLoaded() const;

    void setJustLaunchedWithImage(bool value);

    QScreen *screenContaining(const QRect &rect);

    const QJsonObject getSessionState() const;

    void loadSessionState(const QJsonObject &state, const bool isInitialPhase);

    void openRecent(int i);

    void openUrl(const QUrl &url);

    void pickUrl();

    void reloadFile();

    void openContainingFolder();

    void openWith(const OpenWith::OpenWithItem &exec);

    void showFileInfo();

    void askDeleteFile(bool permanent = false);

    void deleteFile(bool permanent);

    QString deleteFileLinuxFallback(const QString &path, bool putBack);

    void undoDelete();

    void copy();

    void paste();

    void rename();

    void zoomIn();

    void zoomOut();

    void setZoomToFit(const bool value);

    void setFillWindow(const bool value);

    void setNavigationResetsZoom(const bool value);

    void originalSize();

    void rotateRight();

    void rotateLeft();

    void mirror();

    void flip();

    void firstFile();

    void previousFile();

    void nextFile();

    void lastFile();

    void randomFile();

    void saveFrameAs();

    void pause();

    void nextFrame();

    void decreaseSpeed();

    void resetSpeed();

    void increaseSpeed();

    void toggleFullScreen();

    void toggleTitlebarHidden();

    int getTitlebarOverlap() const;

    const QVImageCore::FileDetails& getCurrentFileDetails() const { return graphicsView->getCurrentFileDetails(); }

    qint64 getLastActivatedTimestamp() const { return lastActivated.msecsSinceReference(); }

    bool getIsClosing() const { return isClosing; }

public slots:
    void openFile(const QString &fileName);

    void toggleSlideshow();

    void slideshowAction();

    void cancelSlideshow();

    void fileChanged(const bool isRestoringState);

    void zoomLevelChanged();

    void syncCalculatedZoomMode();

    void syncNavigationResetsZoom();

    void disableActions();

protected:
    bool event(QEvent *event) override;

    void contextMenuEvent(QContextMenuEvent *event) override;

    void showEvent(QShowEvent *event) override;

    void closeEvent(QCloseEvent *event) override;

    void changeEvent(QEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

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

    QColor customBackgroundColor;
    bool checkerboardBackground {false};
    bool menuBarEnabled {false};

    QJsonObject sessionStateToLoad;
    bool justLaunchedWithImage {false};
    bool isClosing {false};
    QElapsedTimer lastActivated;

    Qt::WindowStates storedWindowState {Qt::WindowNoState};
    bool storedTitlebarHidden {false};

    QNetworkAccessManager networkAccessManager;

    QStack<DeletedPaths> lastDeletedFiles;

    QTimer *populateOpenWithTimer;
    QFutureWatcher<QList<OpenWith::OpenWithItem>> openWithFutureWatcher;
};

#endif // MAINWINDOW_H
