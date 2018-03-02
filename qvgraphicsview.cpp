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
        qDebug() << QString("zooming in");
    }
    else
    {
       qDebug() << QString("zooming out");
    }
}
