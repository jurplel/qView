#include "qvgraphicsview.h"
#include <QDebug>
#include <QWheelEvent>
#include <QGraphicsPixmapItem>

QVGraphicsView::QVGraphicsView(QWidget *parent) : QGraphicsView(parent)
{
    scaleFactor = 0.2;
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
        scale(1+scaleFactor, 1+scaleFactor);
        setCurrentScale(getCurrentScale()+scaleFactor);
    }
    else
    {
       scale(1-scaleFactor, 1-scaleFactor);
       setCurrentScale(getCurrentScale()-scaleFactor);
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

void QVGraphicsView::setScaleFactor(qreal newScaleFactor)
{
    scaleFactor = newScaleFactor;
}

qreal QVGraphicsView::getScaleFactor()
{
    return scaleFactor;
}
