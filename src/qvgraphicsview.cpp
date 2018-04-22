#include "qvgraphicsview.h"
#include <QDebug>
#include <QWheelEvent>
#include <QGraphicsPixmapItem>

QVGraphicsView::QVGraphicsView(QWidget *parent) : QGraphicsView(parent)
{
    scaleFactor = 0.25;
    isPixmapLoaded = false;
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(10);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerExpired()));
}


// Events

void QVGraphicsView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    resetScale(true);
}

void QVGraphicsView::dropEvent(QDropEvent *event)
{
    QGraphicsView::dropEvent(event);
    loadMimeData(event->mimeData());
}

void QVGraphicsView::dragEnterEvent(QDragEnterEvent *event)
{
    QGraphicsView::dragEnterEvent(event);
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void QVGraphicsView::dragMoveEvent(QDragMoveEvent *event)
{
    QGraphicsView::dragMoveEvent(event);
    event->acceptProposedAction();
}

void QVGraphicsView::dragLeaveEvent(QDragLeaveEvent *event)
{
    QGraphicsView::dragLeaveEvent(event);
    event->accept();
}

void QVGraphicsView::enterEvent(QEvent *event)
{
    QGraphicsView::enterEvent(event);
    viewport()->setCursor(Qt::ArrowCursor);
}

void QVGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);
    viewport()->setCursor(Qt::ArrowCursor);
}

void QVGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (!loadedPixmapItem)
        return;

    const int DeltaY = event->angleDelta().y();

    if (DeltaY > 0)
    {
        if (getCurrentScale() >= 8)
        return;
    }
    else
    {
        if (getCurrentScale() <= -4)
        return;
    }

    if (getCurrentScale() < 1.0)
    {
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    }
    else
    {
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    }

    if (((getCurrentScale() < 1.0) || (getCurrentScale() <= (scaleFactor+1) && DeltaY < 0)) && getIsScalingEnabled())
    {
        if (DeltaY > 0)
        {
            scaleExpensively(scaleMode::zoomIn);
            setCurrentScale(getCurrentScale()+scaleFactor);
        }
        else
        {
            if (getCurrentScale() == scaleFactor+1)
            {
                resetScale(true);
                return;
            }
            scaleExpensively(scaleMode::zoomOut);
            setCurrentScale(getCurrentScale()-scaleFactor);
        }
    }
    else
    {
        if (!(loadedPixmapItem->pixmap().height() == loadedPixmap.height()))
        {
            loadedPixmapItem->setPixmap(loadedPixmap);
            fitInViewMarginless(loadedPixmapItem->boundingRect(), Qt::KeepAspectRatio);
            fittedMatrix = transform();
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
    }

    if (getCurrentScale() <= 1.0)
    {
        centerOn(loadedPixmapItem->boundingRect().center());
    }
}


// Functions

void QVGraphicsView::loadMimeData(const QMimeData *mimeData)
{
    if (mimeData->hasUrls())
    {
        QStringList pathList;
        const QList<QUrl> urlList = mimeData->urls();

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
    setIsPixmapLoaded(true);
    loadedPixmapItem->setOffset((50000.0 - loadedPixmap.width()/2), (50000.0 - loadedPixmap.height()/2));
    resetScale(true);
    if (getIsFilteringEnabled())
    {
        loadedPixmapItem->setTransformationMode(Qt::SmoothTransformation);
    }
    else
    {
        loadedPixmapItem->setTransformationMode(Qt::FastTransformation);
    }

    selectedFileInfo = QFileInfo(fileName);

    const QDir fileDir = QDir(selectedFileInfo.path());

    loadedFileFolder = fileDir.entryInfoList(filterList, QDir::Files);

    loadedFileFolderIndex = loadedFileFolder.indexOf(selectedFileInfo);
}

void QVGraphicsView::resetScale(bool timed)
{
    if (!getIsPixmapLoaded())
        return;

    if (!getIsScalingEnabled())
        loadedPixmapItem->setPixmap(loadedPixmap);

    fitInViewMarginless(loadedPixmapItem->boundingRect(), Qt::KeepAspectRatio);

    if (!getIsScalingEnabled())
        return;

    timer->start();
}

void QVGraphicsView::timerExpired()
{
    scaleExpensively(scaleMode::resetScale);
}

void QVGraphicsView::scaleExpensively(scaleMode mode)
{
    if (!getIsPixmapLoaded() || !getIsScalingEnabled())
        return;

    float oldPixmapItemHeight = loadedPixmapItem->pixmap().height();
    QPixmap scaledPixmap;

    switch (mode) {
    case scaleMode::resetScale:
    {
        loadedPixmapItem->setPixmap(loadedPixmap);
        fitInViewMarginless(loadedPixmapItem->boundingRect(), Qt::KeepAspectRatio);
        scaledPixmap = loadedPixmap.scaledToHeight(loadedPixmap.height() * transform().m22(), Qt::SmoothTransformation);
        break;
    }
    case scaleMode::zoomIn:
    {
        scaledPixmap = loadedPixmap.scaledToHeight(oldPixmapItemHeight * (scaleFactor+1), Qt::SmoothTransformation);
        break;
    }
    case scaleMode::zoomOut:
    {
        scaledPixmap = loadedPixmap.scaledToHeight(oldPixmapItemHeight / (scaleFactor+1), Qt::SmoothTransformation);
        break;
    }
    default:
        break;
    }

    loadedPixmapItem->setPixmap(scaledPixmap);
    loadedPixmapItem->boundingRect().setWidth(loadedPixmapItem->pixmap().width());
    loadedPixmapItem->boundingRect().setHeight(loadedPixmapItem->pixmap().height());

    if (mode == scaleMode::resetScale)
    {
        fitInViewMarginless(loadedPixmapItem->boundingRect(), Qt::KeepAspectRatio);
    }
}

void QVGraphicsView::nextFile()
{
    if (loadedFileFolder.isEmpty())
        return;

    if (loadedFileFolder.size()-1 == loadedFileFolderIndex)
    {
        loadedFileFolderIndex = 0;
    }
    else
    {
        loadedFileFolderIndex++;
    }

    const QFileInfo nextImage = loadedFileFolder.value(loadedFileFolderIndex);
    if (!nextImage.isFile())
        return;

    loadFile(nextImage.filePath());
}

void QVGraphicsView::previousFile()
{
    if (loadedFileFolder.isEmpty())
        return;

    if (loadedFileFolderIndex == 0)
    {
        loadedFileFolderIndex = loadedFileFolder.size()-1;
    }
    else
    {
        loadedFileFolderIndex--;
    }

    const QFileInfo previousImage = loadedFileFolder.value(loadedFileFolderIndex);
    if (!previousImage.isFile())
        return;

    loadFile(previousImage.filePath());
}

void QVGraphicsView::fitInViewMarginless(const QRectF &rect, Qt::AspectRatioMode aspectRatioMode)
{
    if (!scene() || rect.isNull())
        return;

    // Reset the view scale to 1:1.
    QRectF unity = matrix().mapRect(QRectF(0, 0, 1, 1));
    if (unity.isEmpty())
        return;
    scale(1 / unity.width(), 1 / unity.height());
    // Find the ideal x / y scaling ratio to fit \a rect in the view.
    int margin = -2;
    QRectF viewRect = viewport()->rect().adjusted(margin, margin, -margin, -margin);
    if (viewRect.isEmpty())
        return;
    QRectF sceneRect = matrix().mapRect(rect);
    if (sceneRect.isEmpty())
        return;
    qreal xratio = viewRect.width() / sceneRect.width();
    qreal yratio = viewRect.height() / sceneRect.height();

    // Respect the aspect ratio mode.
    switch (aspectRatioMode) {
    case Qt::KeepAspectRatio:
        xratio = yratio = qMin(xratio, yratio);
        break;
    case Qt::KeepAspectRatioByExpanding:
        xratio = yratio = qMax(xratio, yratio);
        break;
    case Qt::IgnoreAspectRatio:
        break;
    }

    // Scale and center on the center of \a rect.
    scale(xratio, yratio);
    centerOn(rect.center());

    fittedMatrix = transform();
    setCurrentScale(1.0);
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

QFileInfo QVGraphicsView::getSelectedFileInfo() const
{
    return selectedFileInfo;
}

void QVGraphicsView::setSelectedFileInfo(const QFileInfo &value)
{
    selectedFileInfo = value;
}

bool QVGraphicsView::getIsPixmapLoaded() const
{
    return isPixmapLoaded;
}

void QVGraphicsView::setIsPixmapLoaded(bool value)
{
    isPixmapLoaded = value;
}

bool QVGraphicsView::getIsFilteringEnabled() const
{
    return isFilteringEnabled;
}

void QVGraphicsView::setIsFilteringEnabled(bool value)
{
    isFilteringEnabled = value;

    if (getIsPixmapLoaded())
    {
        if (value)
        {
            getLoadedPixmapItem()->setTransformationMode(Qt::SmoothTransformation);
        }
        else
        {
            getLoadedPixmapItem()->setTransformationMode(Qt::FastTransformation);
        }
    }
}

bool QVGraphicsView::getIsScalingEnabled() const
{
    return isScalingEnabled;
}

void QVGraphicsView::setIsScalingEnabled(bool value)
{
    isScalingEnabled = value;
    if (getIsPixmapLoaded())
    {
        resetScale(true);
    }
}
