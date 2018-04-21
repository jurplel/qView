#ifndef QVGRAPHICSVIEW_H
#define QVGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QMimeData>
#include <QDir>

class QVGraphicsView : public QGraphicsView
{

public:
    QVGraphicsView(QWidget *parent = nullptr);

    void loadFile(QString fileName);

    void resetScale();

    void loadMimeData(const QMimeData *mimeData);

    void nextFile();
    void previousFile();

    void viewInFileExplorer();

    qreal getCurrentScale() const;
    void setCurrentScale(const qreal &value);

    qreal getScaleFactor() const;
    void setScaleFactor(const qreal &value);

    QGraphicsPixmapItem *getLoadedPixmapItem() const;
    void setLoadedPixmapItem(QGraphicsPixmapItem *value);

    bool getIsPixmapLoaded() const;
    void setIsPixmapLoaded(bool value);

    QFileInfo getSelectedFileInfo() const;
    void setSelectedFileInfo(const QFileInfo &value);

    bool getIsFilteringEnabled() const;
    void setIsFilteringEnabled(bool value);

protected:
    void wheelEvent(QWheelEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

    void dropEvent(QDropEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;

    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void enterEvent(QEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void fitInViewMarginless(const QRectF &rect, Qt::AspectRatioMode aspectRatioMode);

private:
    qreal currentScale;
    qreal scaleFactor;

    QPixmap loadedPixmap;
    QGraphicsPixmapItem *loadedPixmapItem;
    QTransform fittedMatrix;
    QTransform scaledMatrix;

    bool isPixmapLoaded;
    bool isFilteringEnabled;

    QFileInfo selectedFileInfo;

    QFileInfoList loadedFileFolder;
    int loadedFileFolderIndex;

    const QStringList filterList = (QStringList() << "*.svg" << "*.bmp" << "*.gif" << "*.jpg" << "*.jpeg" << "*.png" << "*.pbm" << "*.pgm" << "*.ppm" << "*.xbm" << "*.xpm");
};
#endif // QVGRAPHICSVIEW_H
