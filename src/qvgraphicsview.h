#ifndef QVGRAPHICSVIEW_H
#define QVGRAPHICSVIEW_H

#include "qvnamespace.h"
#include "qvimagecore.h"
#include "axislocker.h"
#include "scrollhelper.h"
#include <QGraphicsView>
#include <QImageReader>
#include <QMimeData>
#include <QDir>
#include <QTimer>
#include <QFileInfo>

class MainWindow;

class QVGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    QVGraphicsView(QWidget *parent = nullptr);

    enum class GoToFileMode
    {
       constant,
       first,
       previous,
       next,
       last,
       random
    };
    Q_ENUM(GoToFileMode)

    struct SwipeData
    {
        int totalDelta;
        bool triggeredAction;
    };

    QMimeData* getMimeData() const;
    void loadMimeData(const QMimeData *mimeData);
    void loadFile(const QString &fileName);

    void reloadFile();

    void zoomIn(const QPoint &pos = QPoint(-1, -1));

    void zoomOut(const QPoint &pos = QPoint(-1, -1));

    void zoomRelative(const qreal relativeLevel, const QPoint &pos = QPoint(-1, -1));

    void zoomAbsolute(const qreal absoluteLevel, const QPoint &pos = QPoint(-1, -1));

    bool getZoomToFitEnabled() const;
    void setZoomToFitEnabled(bool value);

    bool getNavigationResetsZoomEnabled() const;
    void setNavigationResetsZoomEnabled(bool value);

    void applyExpensiveScaling();
    void removeExpensiveScaling();

    void zoomToFit();
    void originalSize();

    void centerImage();

    const QJsonObject getSessionState() const;

    void loadSessionState(const QJsonObject &state);

    void setLoadIsFromSessionRestore(const bool value);

    void goToFile(const GoToFileMode &mode, int index = 0);

    void settingsUpdated();

    void closeImage();
    void jumpToNextFrame();
    void setPaused(const bool &desiredState);
    void setSpeed(const int &desiredSpeed);
    void rotateImage(const int relativeAngle);
    void mirrorImage();
    void flipImage();

    void fitOrConstrainImage();

    QSizeF getEffectiveOriginalSize() const;

    const QVImageCore::FileDetails& getCurrentFileDetails() const { return imageCore.getCurrentFileDetails(); }
    const QPixmap& getLoadedPixmap() const { return imageCore.getLoadedPixmap(); }
    const QMovie& getLoadedMovie() const { return imageCore.getLoadedMovie(); }
    qreal getZoomLevel() const { return zoomLevel; }

    int getFitOverscan() const { return fitOverscan; }

signals:
    void cancelSlideshow();

    void fileChanged(const bool isRestoringState);

    void zoomLevelChanged();

    void zoomToFitChanged();

    void navigationResetsZoomChanged();

protected:
    void wheelEvent(QWheelEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

    void dropEvent(QDropEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;

    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

    bool event(QEvent *event) override;

    void executeClickAction(const Qv::ViewportClickAction action);

    void executeDragAction(const Qv::ViewportDragAction action, const QPoint delta, bool &isMovingWindow);

    void executeScrollAction(const Qv::ViewportScrollAction action, const QPoint delta, const QPoint mousePos, const bool hasShiftModifier);

    QRectF getContentRect() const;

    QRect getUsableViewportRect(bool addOverscan = false) const;

    void setTransformScale(qreal absoluteScale);

    QTransform getTransformWithNoScaling() const;

    qreal getDpiAdjustment() const;

    void handleDpiAdjustmentChange();

    MainWindow* getMainWindow() const;

private slots:
    void animatedFrameChanged(QRect rect);

    void postLoad();

    void error(int errorNum, const QString &errorString, const QString &fileName);

private:

    QGraphicsPixmapItem *loadedPixmapItem;

    bool isScalingEnabled;
    bool isScalingTwoEnabled;
    bool isPastActualSizeEnabled;
    int fitOverscan;
    bool isScrollZoomsEnabled;
    bool isLoopFoldersEnabled;
    bool isCursorZoomEnabled;
    bool isOneToOnePixelSizingEnabled;
    bool isConstrainedPositioningEnabled;
    bool isConstrainedSmallCenteringEnabled;
    bool sidewaysScrollNavigates;
    Qv::FitMode cropMode;
    qreal zoomMultiplier;

    Qv::ViewportClickAction doubleClickAction {Qv::ViewportClickAction::ToggleFullScreen};
    Qv::ViewportClickAction altDoubleClickAction {Qv::ViewportClickAction::ToggleTitlebarHidden};
    Qv::ViewportClickAction middleClickAction {Qv::ViewportClickAction::ZoomToFit};
    Qv::ViewportClickAction altMiddleClickAction {Qv::ViewportClickAction::OriginalSize};
    Qv::ViewportDragAction dragAction {Qv::ViewportDragAction::Pan};
    Qv::ViewportDragAction altDragAction {Qv::ViewportDragAction::MoveWindow};
    Qv::ViewportScrollAction horizontalScrollAction {Qv::ViewportScrollAction::Navigate};
    Qv::ViewportScrollAction verticalScrollAction {Qv::ViewportScrollAction::Zoom};
    Qv::ViewportScrollAction altHorizontalScrollAction {Qv::ViewportScrollAction::Pan};
    Qv::ViewportScrollAction altVerticalScrollAction {Qv::ViewportScrollAction::Pan};

    bool isZoomToFitEnabled;
    bool isApplyingZoomToFit;
    bool isNavigationResetsZoomEnabled;
    bool loadIsFromSessionRestore;
    qreal zoomLevel;
    qreal appliedDpiAdjustment;
    qreal appliedExpensiveScaleZoomLevel;
    QPoint lastZoomEventPos;
    QPointF lastZoomRoundingError;

    QVImageCore imageCore { this };

    QTimer *expensiveScaleTimer;
    QTimer *constrainBoundsTimer;
    QTimer *emitZoomLevelChangedTimer;

    ScrollHelper *scrollHelper;
    AxisLocker scrollAxisLocker;
    Qt::MouseButton pressedMouseButton;
    Qt::KeyboardModifiers mousePressModifiers;
    QPoint lastMousePos;
};
Q_DECLARE_METATYPE(QVGraphicsView::SwipeData)
#endif // QVGRAPHICSVIEW_H
