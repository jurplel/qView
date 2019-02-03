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
       zoom
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

    void zoom(const int DeltaY, const QPoint pos, qreal targetScaleFactor = 0);

    void loadMimeData(const QMimeData *mimeData);
    void loadFile(const QString &fileName);
    void addRecentFile(const QFileInfo &file);
    void setWindowTitle();

    void resetScale();
    void scaleExpensively(scaleMode mode);
    void originalSize(bool setVariables = true);

    void goToFile(const goToFileMode mode, const int index = 0);

    void loadSettings();

    void jumpToNextFrame();
    void setPaused(const bool &desiredState);
    void setSpeed(const int &desiredSpeed);
    void rotateImage(int rotation);

    const QVImageCore::QVFileDetails& getCurrentFileDetails() const;
    const QPixmap& getLoadedPixmap() const;
    const QMovie& getLoadedMovie() const;

signals:
    void fileLoaded();
    void updatedFileInfo();

    void updateRecentMenu();

    void sendWindowTitle(const QString &newTitle);

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

    void prepareFile();

    void updateFileInfoDisplays();

    void saveRecentFiles();

    void error(const QString &errorString, const QString &fileName);

private:


    QGraphicsPixmapItem *loadedPixmapItem;
    QRectF adjustedBoundingRect;
    QSize adjustedImageSize;

    QTransform fittedMatrix;
    QTransform scaledMatrix;

    bool isFilteringEnabled;
    bool isScalingEnabled;
    bool isScalingTwoEnabled;
    bool isPastActualSizeEnabled;
    bool isScrollZoomsEnabled;
    bool isLoopFoldersEnabled;
    int titlebarMode;
    int cropMode;
    qreal scaleFactor;

    qreal currentScale;
    QSize scaledSize;
    bool isOriginalSize;
    qreal maxScalingTwoSize;
    bool cheapScaledLast;
    bool movieCenterNeedsUpdating;
    QVariantList recentFiles;

    QVImageCore imageCore;

    QTimer *expensiveScaleTimer;
    QTimer *recentsSaveTimer;

    const QStringList filterList = (QStringList() << "*.bmp" << "*.cur" << "*.gif" << "*.icns" << "*.ico" << "*.jp2" << "*.jpeg" << "*.jpe" << "*.jpg" << "*.mng" << "*.pbm" << "*.pgm" << "*.png" << "*.ppm" << "*.svg" << "*.svgz" << "*.tif" << "*.tiff" << "*.wbmp" << "*.webp" << "*.xbm" << "*.xpm");

};
#endif // QVGRAPHICSVIEW_H
