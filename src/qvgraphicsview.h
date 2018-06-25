#ifndef QVGRAPHICSVIEW_H
#define QVGRAPHICSVIEW_H

#include "mainwindow.h"
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

    void zoom(int DeltaY);

    void loadMimeData(const QMimeData *mimeData);
    void loadFile(QString fileName);
    void updateRecentFiles(QFileInfo file);
    void setWindowTitle();

    void resetScale();
    void scaleExpensively(scaleMode mode);
    void originalSize();

    void nextFile();
    void previousFile();

    qreal getCurrentScale() const;
    void setCurrentScale(const qreal &value);

    bool getIsPixmapLoaded() const;
    void setIsPixmapLoaded(bool value);

    QFileInfo getSelectedFileInfo() const;
    void setSelectedFileInfo(const QFileInfo &value);

    bool getIsFilteringEnabled() const;
    void setIsFilteringEnabled(bool value);

    bool getIsScalingEnabled() const;
    void setIsScalingEnabled(bool value);

    int getTitlebarMode() const;
    void setTitlebarMode(int value);

    int getImageHeight();
    int getImageWidth();

    int getCropMode() const;
    void setCropMode(int value);

    qreal getScaleFactor() const;
    void setScaleFactor(const qreal &value);

protected:
    void wheelEvent(QWheelEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

    void dropEvent(QDropEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;

    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void enterEvent(QEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void fitInViewMarginless();


private slots:
    void timerExpired();

    void animatedFrameChange(QRect rect);

private:

    MainWindow *parentMainWindow;

    qreal currentScale;
    qreal scaleFactor;
    bool isOriginalSize;

    QImageReader *reader;
    QMovie *loadedMovie;
    QPixmap *loadedPixmap;
    QGraphicsPixmapItem *loadedPixmapItem;
    QRectF alternateBoundingBox;

    QTransform fittedMatrix;
    QTransform scaledMatrix;
    QTimer *timer;

    bool movieCenterNeedsUpdating;
    bool isMovieLoaded;
    bool isPixmapLoaded;
    bool isFilteringEnabled;
    bool isScalingEnabled;
    int titlebarMode;
    int cropMode;

    QFileInfo selectedFileInfo;

    QFileInfoList loadedFileFolder;
    int loadedFileFolderIndex;
    const QStringList filterList = (QStringList() << "*.bmp" << "*.cur" << "*.gif" << "*.icns" << "*.ico" << "*.jpeg" << "*.jpe" << "*.jpg" << "*.pbm" << "*.pgm" << "*.png" << "*.ppm" << "*.svg" << "*.svgz" << "*.tif" << "*.tiff" << "*.wbmp" << "*.webp" << "*.xbm" << "*.xpm");
};
#endif // QVGRAPHICSVIEW_H
