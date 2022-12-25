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
    setFrameShape(QFrame::NoFrame);
    setTransformationAnchor(QGraphicsView::NoAnchor);

    // part of a pathetic attempt at gesture support
    grabGesture(Qt::PinchGesture);

    // Scene setup
    auto *scene = new QGraphicsScene(-1000000.0, -1000000.0, 2000000.0, 2000000.0, this);
    setScene(scene);

    // Initialize configurable variables
    isFilteringEnabled = true;
    isScalingEnabled = true;
    isScalingTwoEnabled = true;
    isPastActualSizeEnabled = true;
    isScrollZoomsEnabled = true;
    isLoopFoldersEnabled = true;
    isCursorZoomEnabled = true;
    isConstrainedPositioningEnabled = true;
    isConstrainedSmallCenteringEnabled = true;
    cropMode = 0;
    scaleFactor = 1.25;

    // Initialize other variables
    currentScale = 1.0;
    maxScalingTwoSize = 3;
    lastZoomEventPos = QPoint(-1, -1);
    lastZoomRoundingError = QPointF();

    scrollHelper = new ScrollHelper(this,
        [this](ScrollHelper::Parameters &p)
        {
            p.ContentRect = transform().mapRect(loadedPixmapItem->boundingRect()).toRect();
            p.UsableViewportRect = getUsableViewportRect();
            p.ShouldConstrain = isConstrainedPositioningEnabled;
            p.ShouldCenter = isConstrainedSmallCenteringEnabled;
        });

    connect(&imageCore, &QVImageCore::animatedFrameChanged, this, &QVGraphicsView::animatedFrameChanged);
    connect(&imageCore, &QVImageCore::fileChanged, this, &QVGraphicsView::postLoad);
    connect(&imageCore, &QVImageCore::readError, this, &QVGraphicsView::error);

    expensiveScaleTimer = new QTimer(this);
    expensiveScaleTimer->setSingleShot(true);
    expensiveScaleTimer->setInterval(50);
    connect(expensiveScaleTimer, &QTimer::timeout, this, [this]{scaleExpensively();});

    constrainBoundsTimer = new QTimer(this);
    constrainBoundsTimer->setSingleShot(true);
    constrainBoundsTimer->setInterval(500);
    connect(constrainBoundsTimer, &QTimer::timeout, this, [this]{scrollHelper->constrain();});

    emitZoomLevelChangedTimer = new QTimer(this);
    emitZoomLevelChangedTimer->setSingleShot(true);
    emitZoomLevelChangedTimer->setInterval(50);
    connect(emitZoomLevelChangedTimer, &QTimer::timeout, this, [this]{emit zoomLevelChanged();});

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

void QVGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        pressedMouseButton = Qt::LeftButton;
        viewport()->setCursor(Qt::ClosedHandCursor);
        lastMousePos = event->pos();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void QVGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (pressedMouseButton == Qt::LeftButton)
    {
        pressedMouseButton = Qt::NoButton;
        viewport()->setCursor(Qt::ArrowCursor);
        scrollHelper->constrain();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void QVGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (pressedMouseButton == Qt::LeftButton)
    {
        QPoint mouseDelta = event->pos() - lastMousePos;
        scrollHelper->move(-mouseDelta);
        lastMousePos = event->pos();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
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
//                centerOn(loadedPixmapItem);
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
        qreal xDelta = -event->angleDelta().x() / 2.0;
        qreal yDelta = -event->angleDelta().y() / 2.0;

        if (event->modifiers() & Qt::ShiftModifier)
            std::swap(xDelta, yDelta);

        scrollHelper->move(QPointF(xDelta, yDelta));
        constrainBoundsTimer->start();

        return;
    }

    const int yDelta = event->angleDelta().y();
    const qreal yScale = 120.0;

    if (yDelta == 0)
        return;

    const qreal fractionalWheelClicks = qFabs(yDelta) / yScale;
    const qreal zoomAmountPerWheelClick = scaleFactor - 1.0;
    qreal zoomFactor = 1.0 + (fractionalWheelClicks * zoomAmountPerWheelClick);

    if (yDelta < 0)
        zoomFactor = qPow(zoomFactor, -1);

    zoom(zoomFactor, eventPos);
}

// Functions

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

void QVGraphicsView::loadFile(const QString &fileName)
{
    scrollHelper->cancelAnimation();
    imageCore.loadFile(fileName);
}

void QVGraphicsView::postLoad()
{
    // Set the pixmap to the new image and reset the transform's scale to a known value
    makeUnscaled();

    // Fit and center the new image
    resetScale();

    qvApp->getActionManager().addFileToRecentsList(getCurrentFileDetails().fileInfo);

    emit fileChanged();
}

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
    //don't zoom too far out, dude
    currentScale *= scaleFactor;
    if (currentScale >= 500 || currentScale <= 0.01)
    {
        currentScale *= qPow(scaleFactor, -1);
        return;
    }

    if (pos != lastZoomEventPos)
    {
        lastZoomEventPos = pos;
        lastZoomRoundingError = QPointF();
    }
    const QPointF scenePos = mapToScene(pos) - lastZoomRoundingError;

    scale(scaleFactor, scaleFactor);
    scrollHelper->cancelAnimation();

    // If we have a point to zoom towards and cursor zooming is enabled
    if (pos != QPoint(-1, -1) && isCursorZoomEnabled)
    {
        const QPointF p1mouse = mapFromScene(scenePos);
        const QPointF move = p1mouse - pos;
        horizontalScrollBar()->setValue(move.x() + horizontalScrollBar()->value());
        verticalScrollBar()->setValue(move.y() + verticalScrollBar()->value());
        lastZoomRoundingError = mapToScene(pos) - scenePos;
        constrainBoundsTimer->start();
    }
    else
    {
        centerOn(loadedPixmapItem);
    }

    if (isScalingEnabled)
    {
        expensiveScaleTimer->start();
    }

    emitZoomLevelChangedTimer->start();
}

void QVGraphicsView::setZoomLevel(qreal absoluteScaleFactor)
{
    zoom(absoluteScaleFactor / currentScale);
}

void QVGraphicsView::scaleExpensively()
{
    // If we are above maximum scaling size
    if ((currentScale >= maxScalingTwoSize) ||
        (!isScalingTwoEnabled && currentScale > 1.00001))
    {
        // Return to original size
        makeUnscaled();
        return;
    }

    // Map size of the original pixmap to the scale acquired in fitting with modification from zooming percentage
    const QSizeF mappedPixmapSize = QSizeF(getCurrentFileDetails().loadedPixmapSize) * currentScale * devicePixelRatioF();

    // Set image to scaled version
    loadedPixmapItem->setPixmap(imageCore.scaleExpensively(mappedPixmapSize));

    // Set appropriate scale factor
    qreal targetScale = 1.0 / devicePixelRatioF();
    setTransform(getTransformWithNoScaling().scale(targetScale, targetScale));
}

void QVGraphicsView::makeUnscaled()
{
    // Return to original size
    if (getCurrentFileDetails().isMovieLoaded)
        loadedPixmapItem->setPixmap(getLoadedMovie().currentPixmap());
    else
        loadedPixmapItem->setPixmap(getLoadedPixmap());

    // Set appropriate scale factor
    setTransform(getTransformWithNoScaling().scale(currentScale, currentScale));
}

void QVGraphicsView::animatedFrameChanged(QRect rect)
{
    Q_UNUSED(rect)

    if (isScalingEnabled)
    {
        scaleExpensively();
    }
    else
    {
        loadedPixmapItem->setPixmap(getLoadedMovie().currentPixmap());
    }
}

void QVGraphicsView::resetScale()
{
    if (!getCurrentFileDetails().isPixmapLoaded)
        return;

    QSizeF effectiveImageSize = getTransformWithNoScaling().mapRect(QRectF(QPointF(), getCurrentFileDetails().loadedPixmapSize)).size();
    QSizeF viewSize = getUsableViewportRect().adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN).size();

    if (viewSize.isEmpty())
        return;

    qreal fitXRatio = viewSize.width() / effectiveImageSize.width();
    qreal fitYRatio = viewSize.height() / effectiveImageSize.height();

    qreal targetRatio;

    switch (cropMode) { // should be enum tbh
    case 1: // only take into account height
        targetRatio = fitYRatio;
        break;
    case 2: // only take into account width
        targetRatio = fitXRatio;
        break;
    default:
        targetRatio = qMin(fitXRatio, fitYRatio);
        break;
    }

    if (targetRatio > 1.0 && !isPastActualSizeEnabled)
        targetRatio = 1.0;

    setZoomLevel(targetRatio);
}

void QVGraphicsView::originalSize()
{
    setZoomLevel(1.0);
}

void QVGraphicsView::goToFile(const GoToFileMode &mode, int index)
{
    bool shouldRetryFolderInfoUpdate = false;

    // Update folder info only after a little idle time as an optimization for when
    // the user is rapidly navigating through files.
    if (!getCurrentFileDetails().timeSinceLoaded.isValid() || getCurrentFileDetails().timeSinceLoaded.hasExpired(3000))
    {
        // Make sure the file still exists because if it disappears from the file listing we'll lose
        // lose track of our index within the folder. Use the static 'exists' method to avoid caching.
        // If we skip updating now, flag it for retry later once we locate a new file.
        if (QFile::exists(getCurrentFileDetails().fileInfo.absoluteFilePath()))
            imageCore.updateFolderInfo();
        else
            shouldRetryFolderInfoUpdate = true;
    }

    if (getCurrentFileDetails().folderFileInfoList.isEmpty())
        return;

    int newIndex = getCurrentFileDetails().loadedIndexInFolder;
    int searchDirection = 0;

    switch (mode) {
    case GoToFileMode::constant:
    {
        newIndex = index;
        break;
    }
    case GoToFileMode::first:
    {
        newIndex = 0;
        searchDirection = 1;
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
        searchDirection = -1;
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
        searchDirection = 1;
        break;
    }
    case GoToFileMode::last:
    {
        newIndex = getCurrentFileDetails().folderFileInfoList.size()-1;
        searchDirection = -1;
        break;
    }
    }

    if (searchDirection != 0)
    {
        const auto &fileList = getCurrentFileDetails().folderFileInfoList;
        while (searchDirection == 1 && newIndex < fileList.size()-1 && !QFileInfo::exists(fileList.value(newIndex).absoluteFilePath))
            newIndex++;
        while (searchDirection == -1 && newIndex > 0 && !QFileInfo::exists(fileList.value(newIndex).absoluteFilePath))
            newIndex--;
    }

    const QString nextImageFilePath = getCurrentFileDetails().folderFileInfoList.value(newIndex).absoluteFilePath;

    if (!QFile::exists(nextImageFilePath) || nextImageFilePath == getCurrentFileDetails().fileInfo.absoluteFilePath())
        return;

    if (shouldRetryFolderInfoUpdate)
        imageCore.updateFolderInfo(nextImageFilePath);

    loadFile(nextImageFilePath);
}

void QVGraphicsView::centerOn(const QPointF &pos)
{
    QRect targetRect = getUsableViewportRect();
    QPointF viewPoint = transform().map(pos);

    if (isRightToLeft())
    {
        qint64 horizontal = 0;
        horizontal += horizontalScrollBar()->minimum();
        horizontal += horizontalScrollBar()->maximum();
        horizontal -= int(viewPoint.x() - targetRect.left() -  (targetRect.width() / 2.0));
        horizontalScrollBar()->setValue(horizontal);
    }
    else
    {
        horizontalScrollBar()->setValue(int(viewPoint.x() - targetRect.left() - (targetRect.width() / 2.0)));
    }

    verticalScrollBar()->setValue(int(viewPoint.y() - targetRect.top() - (targetRect.height() / 2.0)));

    scrollHelper->cancelAnimation();
}

void QVGraphicsView::centerOn(qreal x, qreal y)
{
    centerOn(QPointF(x, y));
}

void QVGraphicsView::centerOn(const QGraphicsItem *item)
{
    centerOn(item->sceneBoundingRect().center());
}

QRect QVGraphicsView::getUsableViewportRect() const
{
#ifdef COCOA_LOADED
    int obscuredHeight = QVCocoaFunctions::getObscuredHeight(window()->windowHandle());
#else
    int obscuredHeight = 0;
#endif
    QRect rect = viewport()->rect();
    rect.setTop(obscuredHeight);
    return rect;
}

QTransform QVGraphicsView::getTransformWithNoScaling()
{
    qreal currentTransformScale = transform().mapRect(QRectF(QPointF(), QSizeF(1, 1))).width();
    return QTransform(transform()).scale(1.0 / currentTransformScale, 1.0 / currentTransformScale);
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
    if (!isScalingEnabled)
        makeUnscaled();

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

    //constrained positioning
    isConstrainedPositioningEnabled = settingsManager.getBoolean("constrainimageposition");

    //constrained small centering
    isConstrainedSmallCenteringEnabled = settingsManager.getBoolean("constraincentersmallimage");

    //loop folders
    isLoopFoldersEnabled = settingsManager.getBoolean("loopfoldersenabled");

    if (getCurrentFileDetails().isPixmapLoaded)
        resetScale();
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
    rotate(rotation);
}
