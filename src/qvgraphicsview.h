#ifndef QVGRAPHICSVIEW_H
#define QVGRAPHICSVIEW_H

#include "qvimagecore.h"
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

    enum class ScaleMode
    {
       resetScale,
       zoom
    };
    Q_ENUM(ScaleMode)

    enum class GoToFileMode
    {
       constant,
       first,
       previous,
       next,
       last
    };
    Q_ENUM(GoToFileMode)

    void zoom(int DeltaY, const QPoint &pos, qreal targetScaleFactor = 0);

    QMimeData* getMimeData() const;
    void loadMimeData(const QMimeData *mimeData);
    void loadFile(const QString &fileName);

    void resetScale();
    void scaleExpensively(ScaleMode mode);
    void originalSize(bool setVariables = true);

    void goToFile(const GoToFileMode &mode, int index = 0);

    void settingsUpdated();

    void jumpToNextFrame();
    void setPaused(const bool &desiredState);
    void setSpeed(const int &desiredSpeed);
    void rotateImage(int rotation);

    const QVImageCore::QVFileDetails& getCurrentFileDetails() const { return imageCore.getCurrentFileDetails(); }
    const QPixmap& getLoadedPixmap() const { return imageCore.getLoadedPixmap(); }
    const QMovie& getLoadedMovie() const { return imageCore.getLoadedMovie(); }

signals:
    void cancelSlideshow();

    void fileLoaded();

    void updatedLoadedPixmapItem();

protected:
    void wheelEvent(QWheelEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

    void dropEvent(QDropEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;

    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void enterEvent(QEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    bool event(QEvent *event) override;

    void fitInViewMarginless(bool setVariables = true);

    void centerOn(const QPointF &pos);

    void centerOn(qreal x, qreal y);

    void centerOn(const QGraphicsItem *item);


private slots:
    void animatedFrameChanged(QRect rect);

    void postLoad();

    void updateLoadedPixmapItem();

    void error(int errorNum, const QString &errorString, const QString &fileName);

private:


    QGraphicsPixmapItem *loadedPixmapItem;
    QRectF adjustedBoundingRect;
    QSize adjustedImageSize;

    QTransform fittedTransform;
    QTransform scaledTransform;

    bool isFilteringEnabled;
    bool isScalingEnabled;
    bool isScalingTwoEnabled;
    bool isPastActualSizeEnabled;
    bool isScrollZoomsEnabled;
    bool isLoopFoldersEnabled;
    bool isCursorZoomEnabled;
    int cropMode;
    qreal scaleFactor;

    qreal currentScale;
    QSize scaledSize;
    bool isOriginalSize;
    qreal maxScalingTwoSize;
    bool cheapScaledLast;
    bool movieCenterNeedsUpdating;

    QVImageCore imageCore;

    QTimer *expensiveScaleTimer;
};
#endif // QVGRAPHICSVIEW_H
