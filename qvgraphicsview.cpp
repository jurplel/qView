#include "qvgraphicsview.h"
#include <QDebug>
#include <QWheelEvent>
#include <QGraphicsPixmapItem>

QVGraphicsView::QVGraphicsView(QWidget *parent) : QGraphicsView(parent)
{

}

void QVGraphicsView::wheelEvent(QWheelEvent *event)
{
    qDebug()<< QString("before scroll scale: ") << getCurrentScale();

    int DeltaY = event->angleDelta().y();

    if (getCurrentScale() >= 1.0)
    {
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    }
    else
    {
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        centerOn(scene()->height()/2, scene()->width()/2);
    }

    if (DeltaY > 0)
    {
        scale(1.2, 1.2);
        setCurrentScale(getCurrentScale()*1.2);
    }
    else
    {
       scale(0.8, 0.8);
       setCurrentScale(getCurrentScale()*0.8);
    }

    qDebug() << QString("after scroll scale: ") << getCurrentScale();
}

void QVGraphicsView::resetScale(QGraphicsPixmapItem* affectedItem)
{
    fitInView(affectedItem->boundingRect(), Qt::KeepAspectRatio);
    setCurrentScale(1.0);
}

void QVGraphicsView::setCurrentScale(qreal newCurrentScale)
{
    currentScale = newCurrentScale;
}

qreal QVGraphicsView::getCurrentScale()
{
    return currentScale;
}
