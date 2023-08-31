#ifndef QVGRAPHICSVIEW_H
#define QVGRAPHICSVIEW_H

#include "qvimagecore.h"
#include "scrollhelper.h"
#include <QGraphicsView>
#include <QImageReader>
#include <QMimeData>
#include <QDir>
#include <QTimer>
#include <QFileInfo>

class QVGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    QVGraphicsView(QWidget *parent = nullptr);

    static const int FitOverscan = 1;

    enum class GoToFileMode
    {
       constant,
       first,
       previous,
       next,
       last
    };
    Q_ENUM(GoToFileMode)


    QMimeData* getMimeData() const;
    void loadMimeData(const QMimeData *mimeData);
    void loadFile(const QString &fileName);

    void reloadFile();

    void zoomIn(const QPoint &pos = QPoint(-1, -1));

    void zoomOut(const QPoint &pos = QPoint(-1, -1));

    void zoom(qreal scaleFactor, const QPoint &pos = QPoint(-1, -1));

    void setZoomLevel(qreal absoluteScaleFactor);

    bool getResizeResetsZoom() const;
    void setResizeResetsZoom(bool value);

    bool getNavResetsZoom() const;
    void setNavResetsZoom(bool value);

    void scaleExpensively();
    void makeUnscaled();

    void zoomToFit();
    void originalSize();

    void centerImage();

    void goToFile(const GoToFileMode &mode, int index = 0);

    void settingsUpdated();

    void closeImage();
    void jumpToNextFrame();
    void setPaused(const bool &desiredState);
    void setSpeed(const int &desiredSpeed);
    void rotateImage(int rotation);

    QSizeF getEffectiveOriginalSize() const;

    const QVImageCore::FileDetails& getCurrentFileDetails() const { return imageCore.getCurrentFileDetails(); }
    const QPixmap& getLoadedPixmap() const { return imageCore.getLoadedPixmap(); }
    const QMovie& getLoadedMovie() const { return imageCore.getLoadedMovie(); }
    qreal getCurrentScale() const { return currentScale; }

signals:
    void cancelSlideshow();

    void fileChanged();

    void zoomLevelChanged();

    void resizeResetsZoomChanged();

    void navResetsZoomChanged();

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

    void keyPressEvent(QKeyEvent *event) override;

    bool event(QEvent *event) override;

    void centerOn(const QPointF &pos);

    void centerOn(qreal x, qreal y);

    void centerOn(const QGraphicsItem *item);

    QRectF getContentRect() const;

    QRect getUsableViewportRect(bool addOverscan = false) const;

    qreal getContentToViewportRatio() const;

    QTransform getTransformWithNoScaling() const;

    qreal getScaleAdjustment() const;

    void handleScaleAdjustmentChange();

private slots:
    void animatedFrameChanged(QRect rect);

    void postLoad();

    void error(int errorNum, const QString &errorString, const QString &fileName);

private:


    QGraphicsPixmapItem *loadedPixmapItem;

    bool isFilteringEnabled;
    bool isScalingEnabled;
    bool isScalingTwoEnabled;
    bool isPastActualSizeEnabled;
    bool isScrollZoomsEnabled;
    bool isLoopFoldersEnabled;
    bool isCursorZoomEnabled;
    bool isOneToOnePixelSizingEnabled;
    bool isConstrainedPositioningEnabled;
    bool isConstrainedSmallCenteringEnabled;
    int cropMode;
    qreal scaleFactor;

    bool resizeResetsZoom;
    bool navResetsZoom;
    qreal currentScale;
    qreal appliedScaleAdjustment;
    QPoint lastZoomEventPos;
    QPointF lastZoomRoundingError;

    QVImageCore imageCore { this };

    QTimer *expensiveScaleTimer;
    QTimer *constrainBoundsTimer;
    QTimer *emitZoomLevelChangedTimer;
    QPointF centerPoint;

    ScrollHelper *scrollHelper;
    Qt::MouseButton pressedMouseButton;
    QPoint lastMousePos;
};
#endif // QVGRAPHICSVIEW_H
