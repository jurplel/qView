#ifndef QVGRAPHICSVIEW_H
#define QVGRAPHICSVIEW_H

#include <QGraphicsView>

class QVGraphicsView : public QGraphicsView
{

public:
    QVGraphicsView(QWidget *parent = nullptr);

    void resetScale(QGraphicsPixmapItem* affectedItem);

    void setCurrentScale(qreal newCurrentScale);
    qreal getCurrentScale();

    void setScaleFactor(qreal newScaleFactor);
    qreal getScaleFactor();

protected:
    virtual void wheelEvent(QWheelEvent *event);

private:
    qreal currentScale;
    qreal scaleFactor;
};
#endif // QVGRAPHICSVIEW_H
