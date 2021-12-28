#include "qvgraphicsview.h"
#include "qvapplication.h"
#include "qvinfodialog.h"
#include "qvcocoafunctions.h"
#include <QWheelEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QSettings>
#include <QMessageBox>
#include <QMovie>
#include <QtMath>
#include <QGestureEvent>
#include <QScrollBar>

QVGraphicsView::QVGraphicsView(QWidget *parent) : QGraphicsView(parent)
{
    // GraphicsView setup
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setFrameShape(QFrame::NoFrame);
    setTransformationAnchor(QGraphicsView::NoAnchor);

    // part of a pathetic attempt at gesture support
    grabGesture(Qt::PinchGesture);

    // Scene setup
    auto *scene = new QGraphicsScene(0.0, 0.0, 100000.0, 100000.0, this);
    setScene(scene);

    // Initialize configurable variables
    isFilteringEnabled = true;
    isScalingEnabled = true;
    isScalingTwoEnabled = true;
    isPastActualSizeEnabled = true;
    isScrollZoomsEnabled = true;
    isLoopFoldersEnabled = true;
    isCursorZoomEnabled = true;
    cropMode = 0;
    scaleFactor = 1.25;

    // Initialize other variables
    adjustedBoundingRect = QRectF();
    adjustedImageSize = QSize();
    currentScale = 1.0;
    scaledSize = QSize();
    maxScalingTwoSize = 3;
    cheapScaledLast = false;
    movieCenterNeedsUpdating = false;
    isOriginalSize = false;

    connect(&imageCore, &QVImageCore::animatedFrameChanged, this, &QVGraphicsView::animatedFrameChanged);
    connect(&imageCore, &QVImageCore::fileChanged, this, &QVGraphicsView::postLoad);
    connect(&imageCore, &QVImageCore::updateLoadedPixmapItem, this, &QVGraphicsView::updateLoadedPixmapItem);
    connect(&imageCore, &QVImageCore::readError, this, &QVGraphicsView::error);

    expensiveScaleTimer = new QTimer(this);
    expensiveScaleTimer->setSingleShot(true);
    expensiveScaleTimer->setInterval(50);
    connect(expensiveScaleTimer, &QTimer::timeout, this, [this]{scaleExpensively(ScaleMode::resetScale);});

    loadedPixmapItem = new QGraphicsPixmapItem();
    scene->addItem(loadedPixmapItem);

    // Connect to settings signal
    connect(&qvApp->getSettingsManager(), &SettingsManager::settingsUpdated, this, &QVGraphicsView::settingsUpdated);
    settingsUpdated();
}


// Events

void QVGraphicsView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (!isOriginalSize)
        resetScale();
    else
        centerOn(loadedPixmapItem);
}

void QVGraphicsView::dropEvent(QDropEvent *event)
{
    QGraphicsView::dropEvent(event);
    loadMimeData(event->mimeData());
}

void QVGraphicsView::dragEnterEvent(QDragEnterEvent *event)
{
    QGraphicsView::dragEnterEvent(event);
    if (event->mimeData()->hasUrls())
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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void QVGraphicsView::enterEvent(QEvent *event)
#else
void QVGraphicsView::enterEvent(QEnterEvent *event)
#endif
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
        auto *gestureEvent = static_cast<QGestureEvent*>(event);
        if (QGesture *pinch = gestureEvent->gesture(Qt::PinchGesture))
        {
            auto *pinchGesture = static_cast<QPinchGesture *>(pinch);
            QPinchGesture::ChangeFlags changeFlags = pinchGesture->changeFlags();

            if (changeFlags & QPinchGesture::ScaleFactorChanged) {
                const QPoint hotPoint = mapFromGlobal(pinchGesture->hotSpot().toPoint());
                zoom(pinchGesture->scaleFactor(), hotPoint);
            }

            // Fun rotation stuff maybe later
//            if (changeFlags & QPinchGesture::RotationAngleChanged) {
//                qreal rotationDelta = pinchGesture->rotationAngle() - pinchGesture->lastRotationAngle();
//                rotate(rotationDelta);
//                centerOn(loadedPixmapItem->boundingRect().center());
//            }
            return true;
        }
    }
    return QGraphicsView::event(event);
}

void QVGraphicsView::wheelEvent(QWheelEvent *event)
{
    #if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    const QPoint eventPos = event->position().toPoint();
    #else
    const QPoint eventPos = event->pos();
    #endif

    //Basically, if you are holding ctrl then it scrolls instead of zooms (the shift bit is for horizontal scrolling)
    bool willZoom = isScrollZoomsEnabled;
    if (event->modifiers() == Qt::ControlModifier || event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
        willZoom = !willZoom;

    if (!willZoom)
    {
        //macos automatically scrolls horizontally while holding shift
#ifndef Q_OS_MACOS
        if (event->modifiers() == (Qt::ControlModifier|Qt::ShiftModifier) || event->modifiers() == Qt::ShiftModifier)
            translate(event->angleDelta().y()/2.0, event->angleDelta().x()/2.0);
        else
#endif
            translate(event->angleDelta().x()/2.0, event->angleDelta().y()/2.0);

        return;
    }

    int angleDelta;

    if (event->angleDelta().y() == 0)
        angleDelta = event->angleDelta().x();
    else
        angleDelta = event->angleDelta().y();

    if (angleDelta > 0)
        zoomIn(eventPos);
    else
        zoomOut(eventPos);
}

// Functions

void QVGraphicsView::zoomIn(const QPoint &pos)
{
    zoom(scaleFactor, pos);
}

void QVGraphicsView::zoomOut(const QPoint &pos)
{
    zoom(qPow(scaleFactor, -1), pos);
}

void QVGraphicsView::zoom(qreal scaleFactor, const QPoint &pos)
{
    const QPointF p0scene = mapToScene(pos);

    currentScale *= scaleFactor;
    scale(scaleFactor, scaleFactor);

    // If we are zooming in, we have a point to zoom towards, the mouse is on top of the viewport, and cursor zooming is enabled
    if (currentScale > 1.00001 && pos != QPoint(-1, -1) && underMouse() && isCursorZoomEnabled)
    {
        const QPointF p1mouse = mapFromScene(p0scene);
        const QPointF move = p1mouse - pos;
        horizontalScrollBar()->setValue(move.x() + horizontalScrollBar()->value());
        verticalScrollBar()->setValue(move.y() + verticalScrollBar()->value());
    }
    else
    {
        centerOn(loadedPixmapItem->boundingRect().center());
    }
}

//void QVGraphicsView::zoom(int DeltaY, const QPoint &pos, qreal targetScaleFactor)
//{
//    //if targetScaleFactor is 0 (default) - use scaleFactor variable
//    if (qFuzzyCompare(targetScaleFactor, 0))
//        targetScaleFactor = scaleFactor;

//    //define variables for later
//    QPointF originalMappedPos = mapToScene(pos);
//    QPointF result;

//    if (!getCurrentFileDetails().isPixmapLoaded)
//        return;

//    //if original size set, cancel zoom and reset scale
//    if (isOriginalSize)
//    {
//        originalSize();
//        return;
//    }

//    //don't zoom too far out, dude
//    if (DeltaY > 0)
//    {
//        if (currentScale >= 500)
//            return;
//        currentScale *= targetScaleFactor;
//    }
//    else
//    {
//        if (currentScale <= 0.01)
//            return;
//        currentScale /= targetScaleFactor;
//    }

//    bool shouldUseScaling = isScalingEnabled;
//    bool shouldUseScaling2 = isScalingTwoEnabled;
//    //Disallow scaling2 if movie is loaded
//    if (getCurrentFileDetails().isMovieLoaded)
//        shouldUseScaling2 = false;


//    //Use scaling up to scale factor 1.0 if we should
//    if ((currentScale < 0.99999 || (currentScale < 1.00001 && DeltaY > 0)) && shouldUseScaling)
//    {
//        //zoom expensively
//        scaleExpensively(ScaleMode::zoom);
//        cheapScaledLast = false;
//    }
//    //Use scaling up to the maximum scalingtwo value if we should
//    else if (currentScale < maxScalingTwoSize && shouldUseScaling2)
//    {
//        //to scale the mouse position with the image, the mouse position is mapped to the graphicsitem,
//        //it's scaled with a transform matrix (setScale), and then mapped back to scene. expensive scaling is done as expected.
//        QPointF doubleMapped = loadedPixmapItem->mapFromScene(originalMappedPos);
//        loadedPixmapItem->setTransformOriginPoint(loadedPixmapItem->boundingRect().topLeft());

//        scaleExpensively(ScaleMode::zoom);
//        if (DeltaY > 0)
//            loadedPixmapItem->setScale(targetScaleFactor);
//        else
//            loadedPixmapItem->setScale(qPow(targetScaleFactor, -1));

//        QPointF tripleMapped = loadedPixmapItem->mapToScene(doubleMapped);
//        loadedPixmapItem->setScale(1.0);

//        //when you are zooming out from high zoom levels and hit the "ScalingTwo" level again,
//        //this does one more matrix zoom and cancels the expensive zoom (needed for smoothness)
//        if (cheapScaledLast && DeltaY < 0)
//            scale(qPow(targetScaleFactor, -1), qPow(targetScaleFactor, -1));
//        else
//            originalMappedPos = tripleMapped;

//        cheapScaledLast = false;
//    }
//    //do regular matrix-based cheap scaling as a last resort
//    else
//    {
//        //Sets the pixmap to full resolution when zooming in without scaling2
//        if (loadedPixmapItem->pixmap().hxeight() != getLoadedPixmap().height() && !isScalingTwoEnabled)
//        {
//            loadedPixmapItem->setPixmap(getLoadedPixmap());
//            fitInViewMarginless(false);
//            originalMappedPos = mapToScene(pos);
//        }

//        //zoom using cheap matrix method
//        if (DeltaY > 0)
//        {
//            scale(targetScaleFactor, targetScaleFactor);
//        }
//        else
//        {
//            scale(qPow(targetScaleFactor, -1), qPow(targetScaleFactor, -1));
//            //when the pixmap is set to full resolution, reset the scale back to the fittedheight when going back to expensive scaling town
//            if (!qFuzzyCompare(loadedPixmapItem->boundingRect().height(), scaledSize.height()) && qFuzzyCompare(currentScale, 1.0) && !shouldUseScaling2 && shouldUseScaling)
//                resetScale();
//        }
//        cheapScaledLast = true;
//    }

//    //if you are zooming in and the mouse is in play, zoom towards the mouse
//    //otherwise, just center the image
//    if (currentScale > 1.00001 && underMouse() && isCursorZoomEnabled)
//    {
//        QPointF transformationDiff = mapToScene(viewport()->rect().center()) - mapToScene(pos);
//        result = originalMappedPos + transformationDiff;
//    }
//    else
//    {
//        result = loadedPixmapItem->boundingRect().center();
//    }
//    centerOn(result);
//}

QMimeData *QVGraphicsView::getMimeData() const
{
    auto *mimeData = new QMimeData();
    if (!getCurrentFileDetails().isPixmapLoaded)
        return mimeData;

    mimeData->setUrls({QUrl::fromLocalFile(imageCore.getCurrentFileDetails().fileInfo.absoluteFilePath())});
    mimeData->setImageData(imageCore.getLoadedPixmap().toImage());
    return mimeData;
}

void QVGraphicsView::loadMimeData(const QMimeData *mimeData)
{
    if (mimeData == nullptr)
        return;

    if (!mimeData->hasUrls())
        return;

    const QList<QUrl> urlList = mimeData->urls();

    bool first = true;
    for (const auto &url : urlList)
    {
        if (first)
        {
            loadFile(url.toString());
            emit cancelSlideshow();
            first = false;
            continue;
        }
        QVApplication::openFile(url.toString());
    }
}

void QVGraphicsView::animatedFrameChanged(QRect rect)
{
    Q_UNUSED(rect)

    if (isScalingEnabled)
    {
        QSize newSize = scaledSize;
        if (currentScale <= 1.0 && !isOriginalSize)
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
    qvApp->getActionManager().addFileToRecentsList(getCurrentFileDetails().fileInfo);

    emit fileChanged();
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

void QVGraphicsView::resetScale()
{
    if (!getCurrentFileDetails().isPixmapLoaded)
        return;

    fitInViewMarginless();

    if (!isScalingEnabled)
        return;

    expensiveScaleTimer->start();
}

void QVGraphicsView::scaleExpensively(ScaleMode mode)
{
    if (!getCurrentFileDetails().isPixmapLoaded || !isScalingEnabled)
        return;

    switch (mode) {
    case ScaleMode::resetScale:
    {
        QVImageCore::ScaleMode coreMode = QVImageCore::ScaleMode::normal;
        switch (cropMode) {
        case 0:
        {
            // figure out if we should resize to width or height depending on the gap between the window chrome and the image itself
            // 4 is added to these numbers to take into account the -2 margin from fitInViewMarginless (kind of a misnomer, eh?)
            QSize windowSize = QSize(static_cast<int>(width()*devicePixelRatioF()), static_cast<int>(height()*devicePixelRatioF()));
            qreal marginWidth = (windowSize.width()-adjustedBoundingRect.width()*transform().m11())+4;
            qreal marginHeight = (windowSize.height()-adjustedBoundingRect.height()*transform().m22())+4;
            if (marginWidth < marginHeight)
                coreMode = QVImageCore::ScaleMode::width;
            else
                coreMode = QVImageCore::ScaleMode::height;
            break;
        }
        case 1:
        {
            coreMode = QVImageCore::ScaleMode::height;
            break;
        }
        case 2:
        {
            coreMode = QVImageCore::ScaleMode::width;
            break;
        }
        }

        QSize windowSize = QSize(static_cast<int>(width()*devicePixelRatioF()), static_cast<int>(height()*devicePixelRatioF()));

        //scale only to actual size if scaling past actual size is disabled
        if (!isPastActualSizeEnabled && adjustedImageSize.width() < windowSize.width() && adjustedImageSize.height() < windowSize.height())
            loadedPixmapItem->setPixmap(imageCore.scaleExpensively(adjustedImageSize.width(), adjustedImageSize.height(), coreMode));
        else
            loadedPixmapItem->setPixmap(imageCore.scaleExpensively(windowSize.width()+4, windowSize.height()+4, coreMode));

        fitInViewMarginless();
        scaledSize = loadedPixmapItem->boundingRect().size().toSize();
        break;
    }
    case ScaleMode::zoom:
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
        loadedPixmapItem->setPixmap(getLoadedMovie().currentPixmap());
    else
        loadedPixmapItem->setPixmap(getLoadedPixmap());

    resetTransform();
    centerOn(loadedPixmapItem);

    scaledSize = getLoadedPixmap().size();

    if (setVariables)
    {
        movieCenterNeedsUpdating = true;
        isOriginalSize = true;
    }
}


void QVGraphicsView::goToFile(const GoToFileMode &mode, int index)
{
    imageCore.updateFolderInfo();
    if (getCurrentFileDetails().folderFileInfoList.isEmpty())
        return;

    int newIndex = getCurrentFileDetails().loadedIndexInFolder;

    switch (mode) {
    case GoToFileMode::constant:
    {
        newIndex = index;
        break;
    }
    case GoToFileMode::first:
    {
        newIndex = 0;
        break;
    }
    case GoToFileMode::previous:
    {
        if (newIndex == 0)
        {
            if (isLoopFoldersEnabled)
                newIndex = getCurrentFileDetails().folderFileInfoList.size()-1;
            else
                emit cancelSlideshow();
        }
        else
            newIndex--;
        break;
    }
    case GoToFileMode::next:
    {
        if (getCurrentFileDetails().folderFileInfoList.size()-1 == newIndex)
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
    case GoToFileMode::last:
    {
        newIndex = getCurrentFileDetails().folderFileInfoList.size()-1;
        break;
    }
    }

    const QFileInfo nextImage = getCurrentFileDetails().folderFileInfoList.value(newIndex);
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
    QRectF unity = transform().mapRect(QRectF(0, 0, 1, 1));
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

#ifdef COCOA_LOADED
    int obscuredHeight = QVCocoaFunctions::getObscuredHeight(window()->windowHandle());
    viewRect.setHeight(viewRect.height()-obscuredHeight);
#endif

    if (viewRect.isEmpty())
        return;

    // Find the ideal x / y scaling ratio to fit \a rect in the view.
    QRectF sceneRect = transform().mapRect(adjustedBoundingRect);
    if (sceneRect.isEmpty())
        return;

    qreal xratio = viewRect.width() / sceneRect.width();
    qreal yratio = viewRect.height() / sceneRect.height();

    xratio = yratio = qMin(xratio, yratio);

    // Scale and center on the center of \a rect.
    scale(xratio, yratio);
    centerOn(adjustedBoundingRect.center());

    fittedTransform = transform();
    isOriginalSize = false;
    cheapScaledLast = false;
    if (setVariables)
    {
        currentScale = 1.0;
    }
}

void QVGraphicsView::centerOn(const QPointF &pos)
{
#ifdef COCOA_LOADED
    int obscuredHeight = QVCocoaFunctions::getObscuredHeight(window()->windowHandle());
#else
    int obscuredHeight = 0;
#endif

    qreal width = viewport()->width();
    qreal height = viewport()->height() - obscuredHeight;
    QPointF viewPoint = transform().map(pos);

    if (isRightToLeft())
    {
        qint64 horizontal = 0;
        horizontal += horizontalScrollBar()->minimum();
        horizontal += horizontalScrollBar()->maximum();
        horizontal -= int(viewPoint.x() - width / 2.0);
        horizontalScrollBar()->setValue(horizontal);
    }
    else
    {
        horizontalScrollBar()->setValue(int(viewPoint.x() - width / 2.0));
    }

    verticalScrollBar()->setValue(int(viewPoint.y() - obscuredHeight - (height / 2.0)));
}

void QVGraphicsView::centerOn(qreal x, qreal y)
{
    centerOn(QPointF(x, y));
}

void QVGraphicsView::centerOn(const QGraphicsItem *item)
{
    centerOn(item->boundingRect().center());
}

void QVGraphicsView::error(int errorNum, const QString &errorString, const QString &fileName)
{
    if (!errorString.isEmpty())
    {
        closeImage();
        QMessageBox::critical(this, tr("Error"), tr("Error occurred opening \"%3\":\n%2 (Error %1)").arg(QString::number(errorNum), errorString, fileName));
        return;
    }
}

void QVGraphicsView::settingsUpdated()
{
    auto &settingsManager = qvApp->getSettingsManager();

    //bgcolor
    QBrush newBrush;
    newBrush.setStyle(Qt::SolidPattern);
    if (!settingsManager.getBoolean("bgcolorenabled"))
    {
        newBrush.setColor(QColor(0, 0, 0, 0));
    }
    else
    {
        QColor newColor;
        newColor.setNamedColor(settingsManager.getString("bgcolor"));
        newBrush.setColor(newColor);
    }
    setBackgroundBrush(newBrush);

    //filtering
    if (settingsManager.getBoolean("filteringenabled"))
        loadedPixmapItem->setTransformationMode(Qt::SmoothTransformation);
    else
        loadedPixmapItem->setTransformationMode(Qt::FastTransformation);

    //scaling
    isScalingEnabled = settingsManager.getBoolean("scalingenabled");

    //scaling2
    if (!isScalingEnabled)
        isScalingTwoEnabled = false;
    else
        isScalingTwoEnabled = settingsManager.getBoolean("scalingtwoenabled");

    //cropmode
    cropMode = settingsManager.getInteger("cropmode");

    //scalefactor
    scaleFactor = settingsManager.getInteger("scalefactor")*0.01+1;

    //resize past actual size
    isPastActualSizeEnabled = settingsManager.getBoolean("pastactualsizeenabled");

    //scrolling zoom
    isScrollZoomsEnabled = settingsManager.getBoolean("scrollzoomsenabled");

    //cursor zoom
    isCursorZoomEnabled = settingsManager.getBoolean("cursorzoom");

    //loop folders
    isLoopFoldersEnabled = settingsManager.getBoolean("loopfoldersenabled");

    if (getCurrentFileDetails().isPixmapLoaded)
    {
        resetScale();
        if (getCurrentFileDetails().isMovieLoaded && getLoadedMovie().state() == QMovie::Running)
            movieCenterNeedsUpdating = true;
    }
}

void QVGraphicsView::closeImage()
{
    imageCore.closeImage();
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
