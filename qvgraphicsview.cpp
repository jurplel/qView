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

void QVGraphicsView::loadFile(QString fileName)
{
    if (fileName.isEmpty())
        return;

    if(!loadedPixmap.load(fileName))
        return;

    if(loadedPixmap.isNull())
        return;

    scene()->clear();
    loadedPixmapItem = scene()->addPixmap(loadedPixmap);
    loadedPixmapItem->setOffset((50000.0 - loadedPixmap.width()/2), (50000.0 - loadedPixmap.height()/2));
    resetScale();
    loadedPixmapItem->setTransformationMode(Qt::SmoothTransformation);
}

void QVGraphicsView::resetScale()
{
    fitInView(loadedPixmapItem->boundingRect(), Qt::KeepAspectRatio);
    setCurrentScale(1.0);
}


// getters & setters

QGraphicsPixmapItem *QVGraphicsView::getLoadedPixmapItem() const
{
    return loadedPixmapItem;
}

void QVGraphicsView::setLoadedPixmapItem(QGraphicsPixmapItem *value)
{
    loadedPixmapItem = value;
}

qreal QVGraphicsView::getScaleFactor() const
{
    return scaleFactor;
}

void QVGraphicsView::setScaleFactor(const qreal &value)
{
    scaleFactor = value;
}

qreal QVGraphicsView::getCurrentScale() const
{
    return currentScale;
}

void QVGraphicsView::setCurrentScale(const qreal &value)
{
    currentScale = value;
}
