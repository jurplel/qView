#ifndef QVGRAPHICSVIEW_H
#define QVGRAPHICSVIEW_H

#include <QGraphicsView>

class QVGraphicsView : public QGraphicsView
{

public:
    QVGraphicsView(QWidget *parent = nullptr);

    void resetScale(QGraphicsPixmapItem* affectedItem);

    void setCurrentScale(qreal newScale);
    qreal getCurrentScale();

protected:
    virtual void wheelEvent(QWheelEvent *event);

private:
    qreal currentScale;
};
#endif // QVGRAPHICSVIEW_H
