 #ifndef QVGRAPHICSVIEW_H
#define QVGRAPHICSVIEW_H

#include "mainwindow.h"
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

    enum class scaleMode
    {
       resetScale,
       zoomIn,
       zoomOut
    };
    Q_ENUM(scaleMode)

    enum class goToFileMode
    {
       constant,
       first,
       previous,
       next,
       last
    };
    Q_ENUM(goToFileMode)

    void zoom(int DeltaY, QPoint pos = QPoint());

    void loadMimeData(const QMimeData *mimeData);
    void loadFile(const QString &fileName);
    void updateRecentFiles(const QFileInfo &file);
    void setWindowTitle();

    void resetScale();
    void scaleExpensively(scaleMode mode);
    void originalSize();

    void goToFile(const goToFileMode mode, const int index = 0);

    void loadSettings();

    void jumpToNextFrame();
    void setPaused(const bool &desiredState);
    void setSpeed(const int &desiredSpeed);

    const QVImageCore::QVFileDetails& getCurrentFileDetails() const;
    const QPixmap& getLoadedPixmap() const;
    const QMovie& getLoadedMovie() const;

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


private slots:
    void animatedFrameChanged(QRect rect);

private:


    QGraphicsPixmapItem *loadedPixmapItem;
    QRectF alternateBoundingBox;

    QTransform fittedMatrix;
    QTransform scaledMatrix;

    bool isFilteringEnabled;
    bool isScalingEnabled;
    bool isScalingTwoEnabled;
    bool isPastActualSizeEnabled;
    bool isScrollZoomsEnabled;
    int titlebarMode;
    int cropMode;
    qreal scaleFactor;

    qreal currentScale;
    qreal fittedWidth;
    qreal fittedHeight;
    bool isOriginalSize;
    qreal maxScalingTwoSize;
    bool cheapScaledLast;
    bool movieCenterNeedsUpdating;

    QVImageCore imageCore;

    QTimer *timer;

    MainWindow *parentMainWindow;

    const QStringList filterList = (QStringList() << "*.bmp" << "*.cur" << "*.gif" << "*.icns" << "*.ico" << "*.jp2" << "*.jpeg" << "*.jpe" << "*.jpg" << "*.mng" << "*.pbm" << "*.pgm" << "*.png" << "*.ppm" << "*.svg" << "*.svgz" << "*.tif" << "*.tiff" << "*.wbmp" << "*.webp" << "*.xbm" << "*.xpm");

};
#endif // QVGRAPHICSVIEW_H
