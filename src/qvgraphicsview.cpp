#include "qvgraphicsview.h"
#include "qvapplication.h"
#include <QWheelEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QSettings>
#include <QMessageBox>
#include <QMovie>
#include <QPixmapCache>
#include <QDebug>
#include <QtMath>

QVGraphicsView::QVGraphicsView(QWidget *parent) : QGraphicsView(parent)
{
    //qgraphicsscene setup
    QGraphicsScene *scene = new QGraphicsScene(0.0, 0.0, 100000.0, 100000.0, this);
    setScene(scene);

    scaleFactor = 0.25;
    maxScalingTwoSize = 4;
    cheapScaledLast = false;
    isPixmapLoaded = false;
    movieCenterNeedsUpdating = false;
    isMovieLoaded = false;
    isOriginalSize = false;
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(10);
    connect(timer, &QTimer::timeout, this, &QVGraphicsView::timerExpired);

    parentMainWindow = (static_cast<MainWindow*>(parentWidget()->parentWidget()));

    reader = new QImageReader();
    reader->setDecideFormatFromContent(true);

    loadedPixmap = new QPixmap();
    loadedPixmapItem = new QGraphicsPixmapItem(*loadedPixmap);
    scene->addItem(loadedPixmapItem);
}


// Events

void QVGraphicsView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    resetScale();
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

void QVGraphicsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QGraphicsView::mouseDoubleClickEvent(event);
    parentMainWindow->toggleFullScreen();
}

void QVGraphicsView::wheelEvent(QWheelEvent *event)
{
    zoom(event->angleDelta().y(), event->pos());
}

// Functions

void QVGraphicsView::zoom(int DeltaY, QPoint pos)
{
    QPointF originalMappedPos = mapToScene(pos);
    QPointF result;

    if (!getIsPixmapLoaded())
        return;

    //if original size set, cancel zoom and reset scale
    if (isOriginalSize)
    {
        resetScale();
        return;
    }

    //don't zoom too far out, dude
    if (DeltaY > 0)
    {
        if (getCurrentScale() >= 500)
            return;
        setCurrentScale(getCurrentScale()*scaleFactor);
    }
    else
    {
        if (getCurrentScale() <= 0.01)
            return;
        setCurrentScale(getCurrentScale()/scaleFactor);
    }

    bool veto = false;
    //Disallow zooming expensively while movie is paused
    if (isMovieLoaded)
    {
        if (!(loadedMovie->state() == QMovie::Running))
            veto = true;
    }

    //this is a disaster of an if statement, my apologies but here is what it does:
    //use scaleExpensively if the scale is below 1 (also scaling must be enabled and it must not be a paused movie)
    if (getCurrentScale() < 0.99999 && getIsScalingEnabled() && !veto)
    {
        //zoom expensively
        if (DeltaY > 0)
        {
            scaleExpensively(scaleMode::zoomIn);
        }
        else
        {
            scaleExpensively(scaleMode::zoomOut);
        }
        cheapScaledLast = false;
    }
    //check if expensive scaling while zooming in is enabled and valid
    else if (getCurrentScale() < maxScalingTwoSize && getIsScalingEnabled() && getIsScalingTwoEnabled() && !isMovieLoaded)
    {
        //to scale the mouse position with the image, the mouse position is mapped to the graphicsitem,
        //it's scaled with a matrix (setScale), and then mapped back to scene. expensive scaling is done as expected.
        QPointF doubleMapped = loadedPixmapItem->mapFromScene(originalMappedPos);
        loadedPixmapItem->setTransformOriginPoint(loadedPixmapItem->boundingRect().topLeft());
        if (DeltaY > 0)
        {
            scaleExpensively(scaleMode::zoomIn);
            loadedPixmapItem->setScale(scaleFactor);
        }
        else
        {
            scaleExpensively(scaleMode::zoomOut);
            loadedPixmapItem->setScale(qPow(scaleFactor, -1));
        }
        QPointF tripleMapped = loadedPixmapItem->mapToScene(doubleMapped);
        loadedPixmapItem->setScale(1.0);

        //when you are zooming out from high zoom levels and hit the "ScalingTwo" level again,
        //this does one more matrix zoom and cancels the expensive zoom (needed for smoothness)
        if (cheapScaledLast && DeltaY < 0)
            scale(qPow(scaleFactor, -1), qPow(scaleFactor, -1));
        else
            originalMappedPos = tripleMapped;

        cheapScaledLast = false;
    }
    //do regular matrix-based cheap scaling instead
    else
    {
        //if the pixmap being displayed is not the full resolution, set it to be
        if (!(loadedPixmapItem->pixmap().height() == loadedPixmap->height()) && !isMovieLoaded && !getIsScalingTwoEnabled())
        {
            loadedPixmapItem->setPixmap(*loadedPixmap);
            fitInViewMarginless();
            originalMappedPos = mapToScene(pos);
        }

        //zoom using cheap matrix method
        if (DeltaY > 0)
            scale(scaleFactor, scaleFactor);
        else
            scale(qPow(scaleFactor, -1), qPow(scaleFactor, -1));
        cheapScaledLast = true;
    }

    //if you are zooming in and the mouse is in play, zoom towards the mouse
    //otherwise, just center the image
    if (getCurrentScale() > 1.00001 && underMouse())
    {
        QPointF transformationDiff = mapToScene(viewport()->rect().center()) - mapToScene(pos);
        result = originalMappedPos + transformationDiff;
    }
    else
    {
        result = loadedPixmapItem->boundingRect().center();
    }
    centerOn(result);
}

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

void QVGraphicsView::animatedFrameChange(QRect rect)
{
    if (!isMovieLoaded)
        return;

    loadedPixmapItem->setPixmap(loadedMovie->currentPixmap());

    if (getCurrentScale() == 1.0 && !isOriginalSize)
    {
        fitInViewMarginless();
    }
    if (movieCenterNeedsUpdating)
    {
        movieCenterNeedsUpdating = false;
        centerOn(loadedPixmapItem);
    }
}

void QVGraphicsView::loadFile(QString fileName)
{
    QPixmapCache::clear();
    reader->setFileName(fileName);
    //error checks
    if (!QFile::exists(fileName))
    {
        QMessageBox::critical(this, tr("qView Error"), tr("Error: File does not exist"));
        updateRecentFiles(QFileInfo(fileName));
        return;
    }
    else if (fileName.isEmpty())
    {
        QMessageBox::critical(this, tr("qView Error"), tr("Error: No filepath given"));
        return;
    }
    else if (!loadedPixmap->convertFromImage(reader->read()))
    {
        QMessageBox::critical(this, tr("qView Error"), tr("Error: Failed to load file"));
        if (getIsPixmapLoaded())
            loadFile(selectedFileInfo.filePath());
        return;
    }
    else if (loadedPixmap->isNull())
    {
        QMessageBox::critical(this, tr("qView Error"), tr("Error: Null pixmap"));
        if (getIsPixmapLoaded())
            loadFile(selectedFileInfo.filePath());
        return;
    }

    //animation detection
    if (reader->supportsAnimation() && reader->imageCount() > 1)
    {
        loadedMovie = new QMovie(fileName);
        connect(loadedMovie, &QMovie::updated, this, &QVGraphicsView::animatedFrameChange);
        loadedMovie->start();
        isMovieLoaded = true;
        movieCenterNeedsUpdating = true;
    }
    else if (isMovieLoaded)
    {
        isMovieLoaded = false;
        movieCenterNeedsUpdating = false;
        loadedMovie->deleteLater();
    }

    //set pixmap, setoffset, and move graphicsitem
    loadedPixmapItem->setPixmap(*loadedPixmap);
    loadedPixmapItem->setOffset((scene()->width()/2 - loadedPixmap->width()/2), (scene()->height()/2 - loadedPixmap->height()/2));
    setIsPixmapLoaded(true);

    //set filtering mode
    if (getIsFilteringEnabled())
        loadedPixmapItem->setTransformationMode(Qt::SmoothTransformation);
    else
        loadedPixmapItem->setTransformationMode(Qt::FastTransformation);

    //define info variables
    selectedFileInfo = QFileInfo(fileName);
    loadedFileFolder = QDir(selectedFileInfo.path()).entryInfoList(filterList, QDir::Files);
    loadedFileFolderIndex = loadedFileFolder.indexOf(selectedFileInfo);

    //post-load operations
    resetScale();
    parentMainWindow->refreshProperties();
    updateRecentFiles(selectedFileInfo);
    setWindowTitle();
}

void QVGraphicsView::updateRecentFiles(QFileInfo file)
{
    QSettings settings;
    QVariantList recentFiles = settings.value("recentFiles").value<QVariantList>();
    QStringList fileInfo;

    fileInfo << file.fileName() << file.filePath();

    recentFiles.removeAll(fileInfo);
    if (QFile::exists(file.filePath()))
        recentFiles.prepend(fileInfo);

    if(recentFiles.size() > 10)
    {
        recentFiles.removeLast();
    }

    settings.setValue("recentFiles", recentFiles);
}

void QVGraphicsView::setWindowTitle()
{
    if (!getIsPixmapLoaded())
        return;

    switch (getTitlebarMode()) {
    case 0:
    {
        parentMainWindow->setWindowTitle("qView");
        break;
    }
    case 1:
    {
        parentMainWindow->setWindowTitle(selectedFileInfo.fileName());
        break;
    }
    case 2:
    {
        QLocale locale;
        parentMainWindow->setWindowTitle("qView - "  + selectedFileInfo.fileName() + " - " + QString::number(loadedPixmap->width()) + "x" + QString::number(loadedPixmap->height()) + " - " + locale.formattedDataSize(selectedFileInfo.size()));
        break;
    }
    default:
        break;
    }
}

void QVGraphicsView::resetScale()
{
    if (!getIsPixmapLoaded())
        return;

    if (!getIsScalingEnabled() && !isMovieLoaded)
        loadedPixmapItem->setPixmap(*loadedPixmap);

    fitInViewMarginless();

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

    switch (mode) {
    case scaleMode::resetScale:
    {
        if (isMovieLoaded)
        {
            QSize size = QSize(loadedPixmap->width(), loadedPixmap->height());
            size.scale(width()+4, height()+4, Qt::KeepAspectRatio);
            loadedMovie->setScaledSize(size);
            movieCenterNeedsUpdating = true;
        }
        else
        {
            //4 is added to these numbers to take into account the -2 margin from fitInViewMarginless (kind of a misnomer, eh?)
            qreal marginHeight = (height()-alternateBoundingBox.height()*transform().m22())+4;
            qreal marginWidth = (width()-alternateBoundingBox.width()*transform().m11())+4;
            if (marginWidth < marginHeight)
                loadedPixmapItem->setPixmap(loadedPixmap->scaledToWidth(width()+4, Qt::SmoothTransformation));
            else
                loadedPixmapItem->setPixmap(loadedPixmap->scaledToHeight(height()+4, Qt::SmoothTransformation));
            fitInViewMarginless();
        }
        break;
    }
    case scaleMode::zoomIn:
    {
        QSize size = QSize(loadedPixmap->width(), loadedPixmap->height());
        size.scale(static_cast<int>(fittedWidth * (getCurrentScale())), static_cast<int>(fittedHeight * (getCurrentScale())), Qt::KeepAspectRatio);
        if (isMovieLoaded)
        {

            loadedMovie->setScaledSize(size);
            movieCenterNeedsUpdating = true;
        }
        else
        {
            loadedPixmapItem->setPixmap(loadedPixmap->scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        break;
    }
    case scaleMode::zoomOut:
    {
        QSize size = QSize(loadedPixmap->width(), loadedPixmap->height());
        size.scale(static_cast<int>(fittedWidth * (getCurrentScale())), static_cast<int>(fittedHeight * (getCurrentScale())), Qt::KeepAspectRatio);
        if (isMovieLoaded)
        {
            loadedMovie->setScaledSize(size);
            movieCenterNeedsUpdating = true;
        }
        else
        {
            loadedPixmapItem->setPixmap(loadedPixmap->scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        break;
    }
    }
}

void QVGraphicsView::originalSize()
{
    movieCenterNeedsUpdating = true;
    isOriginalSize = true;
    if (isMovieLoaded)
    {
        loadedMovie->setScaledSize(loadedPixmap->size());
    }
    else
    {
        loadedPixmapItem->setPixmap(*loadedPixmap);
    }
    resetMatrix();
    centerOn(loadedPixmapItem->boundingRect().center());
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

void QVGraphicsView::fitInViewMarginless()
{
    alternateBoundingBox = loadedPixmapItem->boundingRect();
    switch (getCropMode()) {
    case 1:
    {
        alternateBoundingBox.translate(alternateBoundingBox.width()/2, 0);
        alternateBoundingBox.setWidth(1);
        break;
    }
    case 2:
    {
        alternateBoundingBox.translate(0, alternateBoundingBox.height()/2);
        alternateBoundingBox.setHeight(1);
        break;
    }
    }

    if (!scene() || alternateBoundingBox.isNull())
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
    QRectF sceneRect = matrix().mapRect(alternateBoundingBox);
    if (sceneRect.isEmpty())
        return;
    qreal xratio = viewRect.width() / sceneRect.width();
    qreal yratio = viewRect.height() / sceneRect.height();

    xratio = yratio = qMin(xratio, yratio);

    // Scale and center on the center of \a rect.
    scale(xratio, yratio);
    centerOn(alternateBoundingBox.center());

    fittedMatrix = transform();
    isOriginalSize = false;
    cheapScaledLast = false;
    fittedHeight = loadedPixmapItem->boundingRect().height();
    fittedWidth = loadedPixmapItem->boundingRect().width();
    setCurrentScale(1.0);
}

// Getters & Setters

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
            loadedPixmapItem->setTransformationMode(Qt::SmoothTransformation);
        }
        else
        {
            loadedPixmapItem->setTransformationMode(Qt::FastTransformation);
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
}

int QVGraphicsView::getTitlebarMode() const
{
    return titlebarMode;
}

void QVGraphicsView::setTitlebarMode(int value)
{
    titlebarMode = value;
    setWindowTitle();
}

int QVGraphicsView::getImageHeight()
{
    if (getIsPixmapLoaded())
    {
        return loadedPixmap->height();
    }
    else
    {
        return 0;
    }
}

int QVGraphicsView::getImageWidth()
{
    if (getIsPixmapLoaded())
    {
        return loadedPixmap->width();
    }
    else
    {
        return 0;
    }
}

int QVGraphicsView::getCropMode() const
{
    return cropMode;
}

void QVGraphicsView::setCropMode(int value)
{
    cropMode = value;
}

qreal QVGraphicsView::getScaleFactor() const
{
    return scaleFactor;
}

void QVGraphicsView::setScaleFactor(const qreal &value)
{
    scaleFactor = value;
}

bool QVGraphicsView::getIsMovieLoaded() const
{
    return isMovieLoaded;
}

void QVGraphicsView::setIsMovieLoaded(bool value)
{
    isMovieLoaded = value;
}

QMovie *QVGraphicsView::getLoadedMovie() const
{
    return loadedMovie;
}

void QVGraphicsView::setLoadedMovie(QMovie *value)
{
    loadedMovie = value;
}

bool QVGraphicsView::getIsScalingTwoEnabled() const
{
    return isScalingTwoEnabled;
}

void QVGraphicsView::setIsScalingTwoEnabled(bool value)
{
    isScalingTwoEnabled = value;
}
