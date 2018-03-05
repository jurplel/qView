#include "qvgraphicsview.h"
#include <QDebug>
#include <QWheelEvent>

QVGraphicsView::QVGraphicsView(QWidget *parent) : QGraphicsView(parent)
{

}

void QVGraphicsView::wheelEvent(QWheelEvent *event)
{
    qDebug() << event->angleDelta();

    int DeltaY = event->angleDelta().y();
    QPoint pos = event->pos();

    if (DeltaY > 0)
    {
        scale(1.1, 1.1);
    }
    else
    {
       scale(0.9, 0.9);
    }
    centerOn(scene()->height()/2, scene()->width()/2);
}
