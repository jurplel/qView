#ifndef QVGRAPHICSVIEW_H
#define QVGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QMimeData>

class QVGraphicsView : public QGraphicsView
{

public:
    QVGraphicsView(QWidget *parent = nullptr);

    void loadFile(QString fileName);

    void resetScale();

    void loadMimeData(const QMimeData *mimeData);

    qreal getCurrentScale() const;
    void setCurrentScale(const qreal &value);

    qreal getScaleFactor() const;
    void setScaleFactor(const qreal &value);

    QGraphicsPixmapItem *getLoadedPixmapItem() const;
    void setLoadedPixmapItem(QGraphicsPixmapItem *value);

protected:
    virtual void wheelEvent(QWheelEvent *event);

    void resizeEvent(QResizeEvent *event);

    void dropEvent(QDropEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;

    void dragLeaveEvent(QDragLeaveEvent *event) override;

private:
    qreal currentScale;
    qreal scaleFactor;

    QPixmap loadedPixmap;
    QGraphicsPixmapItem *loadedPixmapItem;
    QTransform fittedMatrix;
    QTransform scaledMatrix;

    bool isPixmapLoaded;
};
#endif // QVGRAPHICSVIEW_H
