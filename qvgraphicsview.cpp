#include "qvgraphicsview.h"
#include <QDebug>
#include <QWheelEvent>
#include <QGraphicsPixmapItem>

QVGraphicsView::QVGraphicsView(QWidget *parent) : QGraphicsView(parent)
{
    scaleFactor = 0.25;
    isPixmapLoaded = false;
}

void QVGraphicsView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if(isPixmapLoaded)
    {
        resetScale();
    }
}

void QVGraphicsView::dropEvent(QDropEvent *event)
{
    loadMimeData(event->mimeData());
}

void QVGraphicsView::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void QVGraphicsView::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void QVGraphicsView::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

void QVGraphicsView::wheelEvent(QWheelEvent *event)
{
    qDebug()<< QString("before scroll scale: ") << getCurrentScale();
    qDebug()<< QString("before matrix: ") << matrix();

    const int DeltaY = event->angleDelta().y();

    if (getCurrentScale() <= 1.0)
    {
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    }
    else
    {
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    }

    if (DeltaY > 0)
    {
        fittedMatrix = fittedMatrix * (scaleFactor+1);
        setTransform(fittedMatrix);
        setCurrentScale(getCurrentScale()+scaleFactor);
    }
    else
    {
        fittedMatrix = fittedMatrix / (scaleFactor+1);
        setTransform(fittedMatrix);
        setCurrentScale(getCurrentScale()-scaleFactor);
    }

    if (getCurrentScale() <= 1.0)
    {
        centerOn(scene()->height()/2, scene()->width()/2);
    }

    qDebug()<< QString("after matrix: ") << matrix();
    qDebug() << QString("after scroll scale: ") << getCurrentScale();
}

void QVGraphicsView::loadMimeData(const QMimeData *mimeData)
{
    if (mimeData->hasUrls())
    {
        QStringList pathList;
        QList<QUrl> urlList = mimeData->urls();

        for (int i = 0; i < urlList.size() && i < 32; ++i)
        {
            pathList.append(urlList.at(i).toLocalFile());
        }
        if (!pathList.isEmpty())
        {
            loadFile(pathList.first());
        }
    }
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
    isPixmapLoaded = true;
}

void QVGraphicsView::resetScale()
{
    fitInView(loadedPixmapItem->boundingRect(), Qt::KeepAspectRatio);
    setCurrentScale(1.0);
    fittedMatrix = transform();
}


// Getters & Setters

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
