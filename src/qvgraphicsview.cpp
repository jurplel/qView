#include "qvgraphicsview.h"
#include "qvapplication.h"
#include "qvinfodialog.h"
#include <QWheelEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QSettings>
#include <QMessageBox>
#include <QMovie>
#include <QtMath>
#include <QGestureEvent>

QVGraphicsView::QVGraphicsView(QWidget *parent) : QGraphicsView(parent)
{
    grabGesture(Qt::PinchGesture);

    //qgraphicsscene setup
    auto *scene = new QGraphicsScene(0.0, 0.0, 100000.0, 100000.0, this);
    setScene(scene);
    setFocus();

    //initialize configurable variables
    isFilteringEnabled = true;
    isScalingEnabled = true;
    isScalingTwoEnabled = true;
    isPastActualSizeEnabled = true;
    isScrollZoomsEnabled = true;
    isLoopFoldersEnabled = true;
    titlebarMode = 0;
    cropMode = 0;
    scaleFactor = 1.25;

    //initialize other variables
    adjustedBoundingRect = QRectF();
    adjustedImageSize = QSize();
    currentScale = 1.0;
    scaledSize = QSize();
    maxScalingTwoSize = 3;
    cheapScaledLast = false;
    movieCenterNeedsUpdating = false;
    isOriginalSize = false;
    QSettings settings;
    settings.beginGroup("recents");

    connect(&imageCore, &QVImageCore::animatedFrameChanged, this, &QVGraphicsView::animatedFrameChanged);
    connect(&imageCore, &QVImageCore::fileInfoUpdated, this, &QVGraphicsView::updateFileInfoDisplays);
    connect(&imageCore, &QVImageCore::fileRead, this, &QVGraphicsView::postLoad);
    connect(&imageCore, &QVImageCore::updateLoadedPixmapItem, this, &QVGraphicsView::updateLoadedPixmapItem);
    connect(&imageCore, &QVImageCore::readError, this, &QVGraphicsView::error);

    expensiveScaleTimer = new QTimer(this);
    expensiveScaleTimer->setSingleShot(true);
    expensiveScaleTimer->setInterval(10);
    connect(expensiveScaleTimer, &QTimer::timeout, this, [this]{scaleExpensively(scaleMode::resetScale);});

    recentsSaveTimer = new QTimer(this);
    recentsSaveTimer->setSingleShot(true);
    recentsSaveTimer->setInterval(250);
    connect(recentsSaveTimer, &QTimer::timeout, this, &QVGraphicsView::addRecentFiles);

    loadedPixmapItem = new QGraphicsPixmapItem();
    scene->addItem(loadedPixmapItem);

}


// Events

void QVGraphicsView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (!isOriginalSize)
        resetScale();
    else
        centerOn(loadedPixmapItem->boundingRect().center());
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

bool QVGraphicsView::event(QEvent *event)
{
    //this is for touchpad pinch gestures
    if (event->type() == QEvent::Gesture)
    {
        auto *gestureEvent = dynamic_cast<QGestureEvent*>(event);
        if (QGesture *pinch = gestureEvent->gesture(Qt::PinchGesture))
        {
            auto *pinchGesture = dynamic_cast<QPinchGesture *>(pinch);
            QPinchGesture::ChangeFlags changeFlags = pinchGesture->changeFlags();
            if (changeFlags & QPinchGesture::RotationAngleChanged) {
//                qDebug() << "Rotation angle: " << pinchGesture->rotationAngle() << " Last: " << pinchGesture->lastRotationAngle();
                rotateImage(qFloor(pinchGesture->rotationAngle()/90)*90);
            }
            if (changeFlags & QPinchGesture::ScaleFactorChanged) {
//                qDebug() << "Scale factor: " << pinchGesture->scaleFactor() << " Total: " << pinchGesture->totalScaleFactor();
                qreal scaleAmount = (pinchGesture->scaleFactor()-1.0)/(scaleFactor-1);
                if (qFuzzyCompare(scaleAmount, qFabs(scaleAmount)))
                {
                    for (qreal i = scaleAmount; i > 0; --i)
                    {
//                        qDebug() << "zoom #" << i;
                        zoom(120, mapFromGlobal(pinchGesture->hotSpot().toPoint()));
                    }
                }
                else
                {
                    for (qreal i = scaleAmount; i < 0; ++i)
                    {
//                        qDebug() << "zoom #" << i;
                        zoom(-120, mapFromGlobal(pinchGesture->hotSpot().toPoint()));
                    }
                }
            }
            if (changeFlags & QPinchGesture::CenterPointChanged) {
                translate(pinchGesture->lastCenterPoint().x()-pinchGesture->centerPoint().x(), pinchGesture->lastCenterPoint().y()-pinchGesture->centerPoint().y());
            }
        }
    }
    return QGraphicsView::event(event);
}

void QVGraphicsView::wheelEvent(QWheelEvent *event)
{
    //Basically, if you are holding ctrl then it scrolls instead of zooms (the shift bit is for horizontal scrolling)
    bool mode = isScrollZoomsEnabled;
    if (event->modifiers() == Qt::ControlModifier || event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
        mode = !mode;

    if (mode)
    {
        if (event->angleDelta().y() == 0)
            zoom(event->angleDelta().x(), event->pos());
        else
            zoom(event->angleDelta().y(), event->pos());
    }
    else
    {
        //macos automatically scrolls horizontally while holding shift
        #ifndef Q_OS_MACX
        if (event->modifiers() == (Qt::ControlModifier|Qt::ShiftModifier) || event->modifiers() == Qt::ShiftModifier)
            translate(event->angleDelta().y()/2.0, event->angleDelta().x()/2.0);
        else
        #endif
            translate(event->angleDelta().x()/2.0, event->angleDelta().y()/2.0);
    }
}

// Functions

void QVGraphicsView::zoom(const int DeltaY, const QPoint pos, qreal targetScaleFactor)
{
    //if targetScaleFactor is 0 (default) - use scaleFactor variable
    if (qFuzzyCompare(targetScaleFactor, 0))
        targetScaleFactor = scaleFactor;

    //define variables for later
    QPointF originalMappedPos = mapToScene(pos);
    QPointF result;

    if (!getCurrentFileDetails().isPixmapLoaded)
        return;

    //if original size set, cancel zoom and reset scale
    if (isOriginalSize)
    {
        originalSize();
        return;
    }

    //don't zoom too far out, dude
    if (DeltaY > 0)
    {
        if (currentScale >= 500)
            return;
        currentScale *= targetScaleFactor;
    }
    else
    {
        if (currentScale <= 0.01)
            return;
        currentScale /= targetScaleFactor;
    }

    bool shouldUseScaling = isScalingEnabled;
    bool shouldUseScaling2 = isScalingTwoEnabled;
    //Disallow scaling2 if movie is loaded
    if (getCurrentFileDetails().isMovieLoaded)
        shouldUseScaling2 = false;


    //Use scaling up to scale factor 1.0 if we should
    if ((currentScale < 0.99999 || (currentScale < 1.00001 && DeltaY > 0)) && shouldUseScaling)
    {
        //zoom expensively
        scaleExpensively(scaleMode::zoom);
        cheapScaledLast = false;
    }
    //Use scaling up to the maximum scalingtwo value if we should
    else if (currentScale < maxScalingTwoSize && shouldUseScaling2)
    {
        //to scale the mouse position with the image, the mouse position is mapped to the graphicsitem,
        //it's scaled with a matrix (setScale), and then mapped back to scene. expensive scaling is done as expected.
        QPointF doubleMapped = loadedPixmapItem->mapFromScene(originalMappedPos);
        loadedPixmapItem->setTransformOriginPoint(loadedPixmapItem->boundingRect().topLeft());

        scaleExpensively(scaleMode::zoom);
        if (DeltaY > 0)
            loadedPixmapItem->setScale(targetScaleFactor);
        else
            loadedPixmapItem->setScale(qPow(targetScaleFactor, -1));

        QPointF tripleMapped = loadedPixmapItem->mapToScene(doubleMapped);
        loadedPixmapItem->setScale(1.0);

        //when you are zooming out from high zoom levels and hit the "ScalingTwo" level again,
        //this does one more matrix zoom and cancels the expensive zoom (needed for smoothness)
        if (cheapScaledLast && DeltaY < 0)
            scale(qPow(targetScaleFactor, -1), qPow(targetScaleFactor, -1));
        else
            originalMappedPos = tripleMapped;

        cheapScaledLast = false;
    }
    //do regular matrix-based cheap scaling as a last resort
    else
    {
        //Sets the pixmap to full resolution when zooming in without scaling2
        if (loadedPixmapItem->pixmap().height() != getLoadedPixmap().height() && !isScalingTwoEnabled)
        {
            loadedPixmapItem->setPixmap(getLoadedPixmap());
            fitInViewMarginless(false);
            originalMappedPos = mapToScene(pos);
        }

        //zoom using cheap matrix method
        if (DeltaY > 0)
        {
            scale(targetScaleFactor, targetScaleFactor);
        }
        else
        {
            scale(qPow(targetScaleFactor, -1), qPow(targetScaleFactor, -1));
            //when the pixmap is set to full resolution, reset the scale back to the fittedheight when going back to expensive scaling town
            if (!qFuzzyCompare(loadedPixmapItem->boundingRect().height(), scaledSize.height()) && qFuzzyCompare(currentScale, 1.0) && !shouldUseScaling2 && shouldUseScaling)
                resetScale();
        }
        cheapScaledLast = true;
    }

    //if you are zooming in and the mouse is in play, zoom towards the mouse
    //otherwise, just center the image
    if (currentScale > 1.00001 && underMouse())
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

QMimeData *QVGraphicsView::getMimeData() const
{
    auto *mimeData = new QMimeData();
    mimeData->setUrls({QUrl::fromLocalFile(imageCore.getCurrentFileDetails().fileInfo.filePath())});
    mimeData->setImageData(imageCore.getLoadedPixmap().toImage());
    return mimeData;
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
            emit cancelSlideshow();
        }
    }
}

void QVGraphicsView::animatedFrameChanged(QRect rect)
{
    Q_UNUSED(rect)

    if (isScalingEnabled)
    {
        QSize newSize = scaledSize;
        if (currentScale <= 1.0)
            newSize *= currentScale;

        loadedPixmapItem->setPixmap(imageCore.scaleExpensively(newSize));
    }
    else
    {
        QTransform transform;
        transform.rotate(imageCore.getCurrentRotation());

        QImage transformedImage = getLoadedMovie().currentImage().transformed(transform);

        loadedPixmapItem->setPixmap(QPixmap::fromImage(transformedImage));
    }

    if (movieCenterNeedsUpdating)
    {
        movieCenterNeedsUpdating = false;
        centerOn(loadedPixmapItem);
        if (qFuzzyCompare(currentScale, 1.0) && !isOriginalSize)
        {
            fitInViewMarginless();
        }
    }
}

void QVGraphicsView::loadFile(const QString &fileName)
{
    imageCore.loadFile(fileName);
}

void QVGraphicsView::postLoad()
{
    if (getCurrentFileDetails().isMovieLoaded)
        movieCenterNeedsUpdating = true;
    else
        movieCenterNeedsUpdating = false;

    updateLoadedPixmapItem();

    emit fileLoaded();
}

void QVGraphicsView::updateLoadedPixmapItem()
{
    //set pixmap and offset
    loadedPixmapItem->setPixmap(getLoadedPixmap());
    loadedPixmapItem->setOffset((scene()->width()/2 - getLoadedPixmap().width()/2.0), (scene()->height()/2 - getLoadedPixmap().height()/2.0));
    scaledSize = loadedPixmapItem->boundingRect().size().toSize();

    resetScale();
    emit updatedLoadedPixmapItem();
}

void QVGraphicsView::updateFileInfoDisplays()
{
    recentsSaveTimer->start();
    addRecentFileQueue.append(getCurrentFileDetails().fileInfo);

    setWindowTitle();
    emit updatedFileInfo();
}

void QVGraphicsView::addRecentFiles()
{
    QSettings settings;
    settings.beginGroup("recents");

    QVariantList recentFiles = settings.value("recentFiles").value<QVariantList>();
    foreach (QFileInfo file, addRecentFileQueue)
    {
        QStringList fileInfo;

        fileInfo << file.fileName() << file.absoluteFilePath();

        recentFiles.removeAll(fileInfo);
        if (QFile::exists(file.absoluteFilePath()))
            recentFiles.prepend(fileInfo);

        if(recentFiles.size() > 10)
            recentFiles.removeLast();
    }

    addRecentFileQueue.clear();
    settings.setValue("recentFiles", recentFiles);
    emit updateRecentMenu();
}

void QVGraphicsView::setWindowTitle()
{
    if (!getCurrentFileDetails().isPixmapLoaded)
        return;

    QString newString;
    switch (titlebarMode) {
    case 0:
    {
        newString = "qView";
        break;
    }
    case 1:
    {
        newString = getCurrentFileDetails().fileInfo.fileName();
        break;
    }
    case 2:
    {
        newString = "qView - " + QString::number(getCurrentFileDetails().folderIndex+1) + "/" + QString::number(getCurrentFileDetails().folder.count()) + " - " + getCurrentFileDetails().fileInfo.fileName() + " - "  + QString::number(getCurrentFileDetails().imageSize.width()) + "x" + QString::number(getCurrentFileDetails().imageSize.height()) + " - " + QVInfoDialog::formatBytes(getCurrentFileDetails().fileInfo.size());
        break;
    }
    }

    emit sendWindowTitle(newString);
}

void QVGraphicsView::resetScale()
{
    if (!getCurrentFileDetails().isPixmapLoaded)
        return;

    fitInViewMarginless();

    if (!isScalingEnabled)
        return;

    expensiveScaleTimer->start();
}

void QVGraphicsView::scaleExpensively(scaleMode mode)
{
    if (!getCurrentFileDetails().isPixmapLoaded || !isScalingEnabled)
        return;

    switch (mode) {
    case scaleMode::resetScale:
    {
        QVImageCore::scaleMode coreMode = QVImageCore::scaleMode::normal;
        switch (cropMode) {
        case 0:
        {
            // figure out if we should resize to width or height depending on the gap between the window chrome and the image itself
            // 4 is added to these numbers to take into account the -2 margin from fitInViewMarginless (kind of a misnomer, eh?)
            qreal marginWidth = (width()-adjustedBoundingRect.width()*transform().m11())+4;
            qreal marginHeight = (height()-adjustedBoundingRect.height()*transform().m22())+4;
            if (marginWidth < marginHeight)
                coreMode = QVImageCore::scaleMode::width;
            else
                coreMode = QVImageCore::scaleMode::height;
            break;
        }
        case 1:
        {
            coreMode = QVImageCore::scaleMode::height;
            break;
        }
        case 2:
        {
            coreMode = QVImageCore::scaleMode::width;
            break;
        }
        }


        //scale to adjustedimagesize if only scaling to actualimagesize
        if (!isPastActualSizeEnabled && adjustedImageSize.width() < width() && adjustedImageSize.height() < height())
            loadedPixmapItem->setPixmap(imageCore.scaleExpensively(adjustedImageSize.width(), adjustedImageSize.height(), coreMode));
        else
            loadedPixmapItem->setPixmap(imageCore.scaleExpensively(width()+4, height()+4, coreMode));

        fitInViewMarginless();
        scaledSize = loadedPixmapItem->boundingRect().size().toSize();
        break;
    }
    case scaleMode::zoom:
    {
        QSize newSize = scaledSize * currentScale;
        loadedPixmapItem->setPixmap(imageCore.scaleExpensively(newSize));
        break;
    }
    }
    if (getCurrentFileDetails().isMovieLoaded)
        movieCenterNeedsUpdating = true;
}

void QVGraphicsView::originalSize(bool setVariables)
{
    if (isOriginalSize)
    {
        resetScale();
        return;
    }
    if (getCurrentFileDetails().isMovieLoaded)
        imageCore.scaleExpensively(getLoadedPixmap().size());
    else
        loadedPixmapItem->setPixmap(getLoadedPixmap());
    resetTransform();
    centerOn(loadedPixmapItem->boundingRect().center());

    scaledSize = getLoadedPixmap().size();

    if (setVariables)
    {
        movieCenterNeedsUpdating = true;
        isOriginalSize = true;
    }
}


void QVGraphicsView::goToFile(const goToFileMode mode, const int index)
{
    if (getCurrentFileDetails().folder.isEmpty())
        return;

    imageCore.updateFolderInfo();

    int newIndex = getCurrentFileDetails().folderIndex;

    switch (mode) {
    case goToFileMode::constant:
    {
        newIndex = index;
        break;
    }
    case goToFileMode::first:
    {
        newIndex = 0;
        break;
    }
    case goToFileMode::previous:
    {
        if (newIndex == 0)
        {
            if (isLoopFoldersEnabled)
                newIndex = getCurrentFileDetails().folder.size()-1;
            else
                emit cancelSlideshow();
        }
        else
            newIndex--;
        break;
    }
    case goToFileMode::next:
    {
        if (getCurrentFileDetails().folder.size()-1 == newIndex)
        {
            if (isLoopFoldersEnabled)
                newIndex = 0;
            else
                emit cancelSlideshow();
        }
        else
            newIndex++;
        break;
    }
    case goToFileMode::last:
    {
        newIndex = getCurrentFileDetails().folder.size()-1;
        break;
    }
    }

    const QFileInfo nextImage = getCurrentFileDetails().folder.value(newIndex);
    if (!nextImage.isFile())
        return;

    loadFile(nextImage.absoluteFilePath());
}

void QVGraphicsView::fitInViewMarginless(bool setVariables)
{
    adjustedImageSize = getCurrentFileDetails().loadedPixmapSize;
    adjustedBoundingRect = loadedPixmapItem->boundingRect();

    switch (cropMode) {
    case 1:
    {
        adjustedImageSize.setWidth(1);
        adjustedBoundingRect.setWidth(1);
        break;
    }
    case 2:
    {
        adjustedImageSize.setHeight(1);
        adjustedBoundingRect.setHeight(1);
        break;
    }
    }
    adjustedBoundingRect.moveCenter(loadedPixmapItem->boundingRect().center());

    if (!scene() || adjustedBoundingRect.isNull())
        return;

    // Reset the view scale to 1:1.
    QRectF unity = matrix().mapRect(QRectF(0, 0, 1, 1));
    if (unity.isEmpty())
        return;
    scale(1 / unity.width(), 1 / unity.height());

    //resize to window size unless you are meant to stop at the actual size
    QRectF viewRect;
    if (isPastActualSizeEnabled || (adjustedImageSize.width() >= width() || adjustedImageSize.height() >= height()))
    {
        int margin = -2;
        viewRect = viewport()->rect().adjusted(margin, margin, -margin, -margin);
    }
    else
    {
        viewRect = QRect(QPoint(), getCurrentFileDetails().loadedPixmapSize);
        viewRect.moveCenter(rect().center());
    }
    if (viewRect.isEmpty())
        return;
    // Find the ideal x / y scaling ratio to fit \a rect in the view.
    QRectF sceneRect = matrix().mapRect(adjustedBoundingRect);
    if (sceneRect.isEmpty())
        return;
    qreal xratio = viewRect.width() / sceneRect.width();
    qreal yratio = viewRect.height() / sceneRect.height();

    xratio = yratio = qMin(xratio, yratio);

    // Scale and center on the center of \a rect.
    scale(xratio, yratio);
    centerOn(adjustedBoundingRect.center());

    fittedMatrix = transform();
    isOriginalSize = false;
    cheapScaledLast = false;
    if (setVariables)
    {
        currentScale = 1.0;
    }
}

void QVGraphicsView::error(const QString &errorString, const QString &fileName)
{
        if (!errorString.isEmpty())
        {
            QMessageBox::critical(this, tr("Error"), tr("Error ") + errorString);
            if (errorString.at(0) == "1")
            {
                recentsSaveTimer->start();
                addRecentFileQueue.append(QFileInfo(fileName));
            }
            return;
        }
}

void QVGraphicsView::loadSettings()
{
    QSettings settings;
    settings.beginGroup("options");

    //bgcolor
    QBrush newBrush;
    newBrush.setStyle(Qt::SolidPattern);
    if (!((settings.value("bgcolorenabled", true).toBool())))
    {
        newBrush.setColor(QColor(0, 0, 0, 0));
    }
    else
    {
        QColor newColor;
        newColor.setNamedColor(settings.value("bgcolor", "#212121").toString());
        newBrush.setColor(newColor);
    }
    setBackgroundBrush(newBrush);

    //filtering
    isFilteringEnabled = settings.value("filteringenabled", true).toBool();
    if (isFilteringEnabled)
        loadedPixmapItem->setTransformationMode(Qt::SmoothTransformation);
    else
        loadedPixmapItem->setTransformationMode(Qt::FastTransformation);

    //scaling
    isScalingEnabled = settings.value("scalingenabled", true).toBool();

    //scaling2
    if (!isScalingEnabled)
        isScalingTwoEnabled = false;
    else
        isScalingTwoEnabled = settings.value("scalingtwoenabled", true).toBool();

    //titlebar
    titlebarMode = settings.value("titlebarmode", 1).toInt();
    setWindowTitle();

    //cropmode
    cropMode = settings.value("cropmode", 0).toInt();

    //scalefactor
    scaleFactor = settings.value("scalefactor", 25).toInt()*0.01+1;

    //resize past actual size
    isPastActualSizeEnabled = settings.value("pastactualsizeenabled", true).toBool();

    //scrolling zoom
    isScrollZoomsEnabled = settings.value("scrollzoomsenabled", true).toBool();

    //loop folders
    isLoopFoldersEnabled = settings.value("loopfoldersenabled", true).toBool();

    if (getCurrentFileDetails().isPixmapLoaded)
    {
        resetScale();
        if (getCurrentFileDetails().isMovieLoaded && getLoadedMovie().state() == QMovie::Running)
            movieCenterNeedsUpdating = true;
    }

    imageCore.loadSettings();
}

void QVGraphicsView::jumpToNextFrame()
{
    imageCore.jumpToNextFrame();
}

void QVGraphicsView::setPaused(const bool &desiredState)
{
    imageCore.setPaused(desiredState);
}

void QVGraphicsView::setSpeed(const int &desiredSpeed)
{
    imageCore.setSpeed(desiredSpeed);
}

void QVGraphicsView::rotateImage(int rotation)
{
    imageCore.rotateImage(rotation);
}
