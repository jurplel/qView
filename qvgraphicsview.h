#ifndef QVGRAPHICSVIEW_H
#define QVGRAPHICSVIEW_H

#include <QGraphicsView>

class QVGraphicsView : public QGraphicsView
{

public:
    QVGraphicsView(QWidget *parent = nullptr);

    void loadFile(QString fileName);

    void resetScale();

    qreal getCurrentScale() const;
    void setCurrentScale(const qreal &value);

    qreal getScaleFactor() const;
    void setScaleFactor(const qreal &value);

    QGraphicsPixmapItem *getLoadedPixmapItem() const;
    void setLoadedPixmapItem(QGraphicsPixmapItem *value);

protected:
    virtual void wheelEvent(QWheelEvent *event);

private:
    qreal currentScale;
    qreal scaleFactor;

    QPixmap loadedPixmap;
    QGraphicsPixmapItem *loadedPixmapItem;
};
#endif // QVGRAPHICSVIEW_H
