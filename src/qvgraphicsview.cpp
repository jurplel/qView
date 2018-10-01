#include "qvgraphicsview.h"
#include "qvapplication.h"
#include <QWheelEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QSettings>
#include <QMessageBox>
#include <QMovie>
#include <QPixmapCache>
#include <QtMath>

QVGraphicsView::QVGraphicsView(QWidget *parent) : QGraphicsView(parent)
{
    //qgraphicsscene setup
    auto *scene = new QGraphicsScene(0.0, 0.0, 100000.0, 100000.0, this);
    setScene(scene);
    setFocus();

    currentScale = 1.0;
    fittedWidth = 0;
    fittedHeight = 0;
    isFilteringEnabled = true;
    isScalingEnabled = true;
    isScalingTwoEnabled = true;
    isResetOnResizeEnabled = true;
    isPastActualSizeEnabled = true;

    titlebarMode = 0;
    cropMode = 0;

    scaleFactor = 0.25;
    maxScalingTwoSize = 4;
    cheapScaledLast = false;
    movieCenterNeedsUpdating = false;
    isOriginalSize = false;
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    connect(&imageCore, &QVImageCore::animatedFrameChanged, this, &QVGraphicsView::animatedFrameChanged);

    timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(10);
    connect(timer, &QTimer::timeout, this, &QVGraphicsView::timerExpired);

    parentMainWindow = (dynamic_cast<MainWindow*>(parentWidget()->parentWidget()));

    loadedPixmapItem = new QGraphicsPixmapItem(imageCore.getLoadedPixmap());
    scene->addItem(loadedPixmapItem);

}


// Events

void QVGraphicsView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (isResetOnResizeEnabled)
    {
        if (!isOriginalSize)
            resetScale();
        else
            centerOn(loadedPixmapItem->boundingRect().center());
    }
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
    //Basically, if you are holding ctrl then it scrolls instead of zooms (the shift bit is for horizontal scrolling)
    if (event->modifiers() == Qt::ControlModifier)
    {
        if (event->angleDelta().y() > 0)
            translate(0, height()/10);
        else
            translate(0, height()/10*-1);
    }
    else if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
    {
        if (event->angleDelta().x() != 0)
        {
            if (event->angleDelta().x() > 0)
                translate(width()/10, 0);
            else
                translate(width()/10*-1, 0);
        }
        else
        {
            if (event->angleDelta().y() > 0)
                translate(width()/10, 0);
            else
                translate(width()/10*-1, 0);
        }

    }
    else
    {
        if (event->angleDelta().y() == 0)
            zoom(event->angleDelta().x(), event->pos());
        else
            zoom(event->angleDelta().y(), event->pos());
    }
}

// Functions

void QVGraphicsView::zoom(int DeltaY, QPoint pos)
{
    QPointF originalMappedPos = mapToScene(pos);
    QPointF result;

    if (!imageCore.getCurrentFileDetails().isPixmapLoaded)
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
    if (imageCore.getCurrentFileDetails().isMovieLoaded)
    {
        if (!(imageCore.getLoadedMovie().state() == QMovie::Running))
            veto = true;
    }

    //this is a disaster of an if statement, my apologies but here is what it does:
    //use scaleExpensively if the scale is less than 1 or less than or equal to one while scrolling up (also scaling must be enabled and it must not be a paused movie)
    if ((getCurrentScale() < 0.99999 || (getCurrentScale() < 1.00001 && DeltaY > 0)) && getIsScalingEnabled() && !veto)
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
    else if (getCurrentScale() < maxScalingTwoSize && getIsScalingEnabled() && getIsScalingTwoEnabled() && !imageCore.getCurrentFileDetails().isMovieLoaded)
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
        //Sets the pixmap to full resolution when zooming in without scaling2
        if (loadedPixmapItem->pixmap().height() != imageCore.getLoadedPixmap().height() && imageCore.getCurrentFileDetails().isPixmapLoaded  && !getIsScalingTwoEnabled())
        {
            loadedPixmapItem->setPixmap(imageCore.getLoadedPixmap());
            fitInViewMarginless(false);
            originalMappedPos = mapToScene(pos);
        }

        //zoom using cheap matrix method
        if (DeltaY > 0)
        {
            //this prevents a jitter when zooming in very quickly from below to above 1.0 on a movie
            if (imageCore.getCurrentFileDetails().isMovieLoaded && !qFuzzyCompare(imageCore.getLoadedMovie().currentPixmap().height(), fittedHeight))
                imageCore.jumpToNextFrame();
            scale(scaleFactor, scaleFactor);
        }
        else
        {
            scale(qPow(scaleFactor, -1), qPow(scaleFactor, -1));
            //when the pixmap is set to full resolution, reset the scale back to the fittedheight when going back to expensive scaling town
            if (!qFuzzyCompare(loadedPixmapItem->boundingRect().height(), fittedHeight) && qFuzzyCompare(getCurrentScale(), 1.0) && imageCore.getCurrentFileDetails().isMovieLoaded && !getIsScalingTwoEnabled() && getIsScalingEnabled())
                resetScale();
        }
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

void QVGraphicsView::animatedFrameChanged(QRect rect)
{
    Q_UNUSED(rect);
    if (!imageCore.getCurrentFileDetails().isMovieLoaded)
        return;

    loadedPixmapItem->setPixmap(imageCore.getLoadedMovie().currentPixmap());

    if (movieCenterNeedsUpdating)
    {
        movieCenterNeedsUpdating = false;
        centerOn(loadedPixmapItem);
        if (qFuzzyCompare(getCurrentScale(), 1.0) && !isOriginalSize)
            fitInViewMarginless();
    }
}

void QVGraphicsView::loadFile(const QString &fileName)
{
    int errorCode = imageCore.loadFile(fileName);
    if (errorCode >= 0)
    {
        QMessageBox::critical(this, tr("qView Error"), tr("Read Error"));
        return;
    }

    //set pixmap and offset
    loadedPixmapItem->setPixmap(imageCore.getLoadedPixmap());
    loadedPixmapItem->setOffset((scene()->width()/2 - imageCore.getLoadedPixmap().width()/2), (scene()->height()/2 - imageCore.getLoadedPixmap().height()/2));

    //set filtering mode
    if (getIsFilteringEnabled())
        loadedPixmapItem->setTransformationMode(Qt::SmoothTransformation);
    else
        loadedPixmapItem->setTransformationMode(Qt::FastTransformation);

    //post-load operations
    resetScale();
    parentMainWindow->refreshProperties();
    updateRecentFiles(imageCore.getCurrentFileDetails().fileInfo);
    setWindowTitle();
}

void QVGraphicsView::updateRecentFiles(const QFileInfo &file)
{
    QSettings settings;
    auto recentFiles = settings.value("recentFiles").value<QVariantList>();
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
    parentMainWindow->updateRecentMenu();
}

void QVGraphicsView::setWindowTitle()
{
    if (!imageCore.getCurrentFileDetails().isPixmapLoaded)
        return;

    switch (getTitlebarMode()) {
    case 0:
    {
        parentMainWindow->setWindowTitle("qView");
        break;
    }
    case 1:
    {
        parentMainWindow->setWindowTitle(imageCore.getCurrentFileDetails().fileInfo.fileName());
        break;
    }
    case 2:
    {
        const QFileInfoList loadedFileFolder = QDir(imageCore.getCurrentFileDetails().fileInfo.path()).entryInfoList(filterList, QDir::Files);
        const QString currentFileIndex = QString::number(loadedFileFolder.indexOf(imageCore.getCurrentFileDetails().fileInfo)+1);
        QLocale locale;
        parentMainWindow->setWindowTitle("qView - " + imageCore.getCurrentFileDetails().fileInfo.fileName() + " - " + currentFileIndex + "/" + QString::number(loadedFileFolder.count()) + " - "  + QString::number(imageCore.getLoadedPixmap().width()) + "x" + QString::number(imageCore.getLoadedPixmap().height()) + " - " + locale.formattedDataSize(imageCore.getCurrentFileDetails().fileInfo.size()));
        break;
    }
    default:
        break;
    }
}

void QVGraphicsView::resetScale()
{
    if (!imageCore.getCurrentFileDetails().isPixmapLoaded)
        return;

    if (!getIsScalingEnabled() && !imageCore.getCurrentFileDetails().isMovieLoaded)
        loadedPixmapItem->setPixmap(imageCore.getLoadedPixmap());

    //if we aren't supposed to resize past the actual size, dont do that (the reason it's so long is to take into account the crop mode)
    if (!getIsPastActualSizeEnabled())
        if ((getCropMode() == 1 && height() >= imageCore.getLoadedPixmap().height()) || (getCropMode() == 2 && width() >= imageCore.getLoadedPixmap().width())
        || (getCropMode() == 0  && height() >= imageCore.getLoadedPixmap().height() && width() >= imageCore.getLoadedPixmap().width()))
        {
            if (!imageCore.getCurrentFileDetails().isMovieLoaded)
                loadedPixmapItem->setPixmap(imageCore.getLoadedPixmap());
            centerOn(loadedPixmapItem->boundingRect().center());
            return;
        }

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
    if (!imageCore.getCurrentFileDetails().isPixmapLoaded || !getIsScalingEnabled())
        return;

    switch (mode) {
    case scaleMode::resetScale:
    {
        // figure out if we should resize to width or height depending on the gap between the window chrome and the image itself
        // 4 is added to these numbers to take into account the -2 margin from fitInViewMarginless (kind of a misnomer, eh?)
        qreal marginWidth = (width()-alternateBoundingBox.width()*transform().m11())+4;
        qreal marginHeight = (height()-alternateBoundingBox.height()*transform().m22())+4;
        QVImageCore::scaleMode mode;
        if (marginWidth < marginHeight)
            mode = QVImageCore::scaleMode::width;
        else
            mode = QVImageCore::scaleMode::height;

        loadedPixmapItem->setPixmap(imageCore.scaleExpensively(width()+4, height()+4, mode));

        if (!imageCore.getCurrentFileDetails().isMovieLoaded)
            fitInViewMarginless();

        break;
    }
    case scaleMode::zoomIn:
    {
        QSize newSize = QSize(static_cast<int>(fittedWidth * (getCurrentScale())), static_cast<int>(fittedHeight * (getCurrentScale())));

        loadedPixmapItem->setPixmap(imageCore.scaleExpensively(newSize));
        if (imageCore.getCurrentFileDetails().isMovieLoaded)
            movieCenterNeedsUpdating = true;
        break;
    }
    case scaleMode::zoomOut:
    {
        QSize newSize = QSize(static_cast<int>(fittedWidth * (getCurrentScale())), static_cast<int>(fittedHeight * (getCurrentScale())));
        loadedPixmapItem->setPixmap(imageCore.scaleExpensively(newSize));

        if (imageCore.getCurrentFileDetails().isMovieLoaded)
            movieCenterNeedsUpdating = true;
        break;
    }
    }
}

void QVGraphicsView::originalSize()
{
    if (isOriginalSize)
    {
        resetScale();
        return;
    }
    movieCenterNeedsUpdating = true;
    isOriginalSize = true;
    if (imageCore.getCurrentFileDetails().isMovieLoaded)
        imageCore.scaleExpensively(imageCore.getLoadedPixmap().size());
    else
        loadedPixmapItem->setPixmap(imageCore.getLoadedPixmap());
    resetMatrix();
    centerOn(loadedPixmapItem->boundingRect().center());

    fittedHeight = loadedPixmapItem->boundingRect().height();
    fittedWidth = loadedPixmapItem->boundingRect().width();
}


void QVGraphicsView::goToFile(const goToFileMode mode, const int index)
{
    QFileInfoList loadedFileFolder = QDir(imageCore.getCurrentFileDetails().fileInfo.path()).entryInfoList(filterList, QDir::Files);
    int loadedFileFolderIndex = loadedFileFolder.indexOf(imageCore.getCurrentFileDetails().fileInfo);

    if (loadedFileFolder.isEmpty())
        return;

    switch (mode) {
    case goToFileMode::constant:
    {
        loadedFileFolderIndex = index;
        break;
    }
    case goToFileMode::first:
    {
        loadedFileFolderIndex = 0;
        break;
    }
    case goToFileMode::previous:
    {
        if (loadedFileFolderIndex == 0)
            loadedFileFolderIndex = loadedFileFolder.size()-1;
        else
            loadedFileFolderIndex--;
        break;
    }
    case goToFileMode::next:
    {
        if (loadedFileFolder.size()-1 == loadedFileFolderIndex)
            loadedFileFolderIndex = 0;
        else
            loadedFileFolderIndex++;
        break;
    }
    case goToFileMode::last:
    {
        loadedFileFolderIndex = loadedFileFolder.size()-1;
        break;
    }
    }

    const QFileInfo nextImage = loadedFileFolder.value(loadedFileFolderIndex);
    if (!nextImage.isFile())
        return;

    loadFile(nextImage.filePath());
}

void QVGraphicsView::fitInViewMarginless(bool setVariables)
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
    if (setVariables)
    {
        fittedHeight = loadedPixmapItem->boundingRect().height();
        fittedWidth = loadedPixmapItem->boundingRect().width();
        setCurrentScale(1.0);
    }
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

bool QVGraphicsView::getIsFilteringEnabled() const
{
    return isFilteringEnabled;
}

void QVGraphicsView::setIsFilteringEnabled(bool value)
{
    isFilteringEnabled = value;

    if (imageCore.getCurrentFileDetails().isPixmapLoaded)
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

bool QVGraphicsView::getIsScalingTwoEnabled() const
{
    return isScalingTwoEnabled;
}

void QVGraphicsView::setIsScalingTwoEnabled(bool value)
{
    isScalingTwoEnabled = value;
}

bool QVGraphicsView::getIsResetOnResizeEnabled() const
{
    return isResetOnResizeEnabled;
}

void QVGraphicsView::setIsResetOnResizeEnabled(bool value)
{
    isResetOnResizeEnabled = value;
}

bool QVGraphicsView::getIsPastActualSizeEnabled() const
{
    return isPastActualSizeEnabled;
}

void QVGraphicsView::setIsPastActualSizeEnabled(bool value)
{
    isPastActualSizeEnabled = value;
}

void QVGraphicsView::jumpToNextFrame()
{
    imageCore.jumpToNextFrame();
}

void QVGraphicsView::setPaused(bool desiredState)
{
    imageCore.setPaused(desiredState);
}

void QVGraphicsView::setSpeed(int desiredSpeed)
{
    imageCore.setSpeed(desiredSpeed);
}

const QVImageCore::QVFileDetails& QVGraphicsView::getCurrentFileDetails() const
{
    return imageCore.getCurrentFileDetails();
}

const QPixmap& QVGraphicsView::getLoadedPixmap() const
{
    return imageCore.getLoadedPixmap();
}

const QMovie& QVGraphicsView::getLoadedMovie() const
{
    return imageCore.getLoadedMovie();
}
