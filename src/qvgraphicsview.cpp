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
    isOneToOnePixelSizingEnabled = true;
    isConstrainedPositioningEnabled = true;
    isConstrainedSmallCenteringEnabled = true;
    cropMode = 0;
    scaleFactor = 1.25;

    // Initialize other variables
    isZoomToFitEnabled = true;
    isApplyingZoomToFit = false;
    isNavigationResetsZoomEnabled = true;
    currentScale = 1.0;
    appliedScaleAdjustment = 1.0;
    lastZoomEventPos = QPoint(-1, -1);
    lastZoomRoundingError = QPointF();

    scrollHelper = new ScrollHelper(this,
        [this](ScrollHelper::Parameters &p)
        {
            p.ContentRect = getContentRect().toRect();
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
    fitOrConstrainImage();
}

void QVGraphicsView::paintEvent(QPaintEvent *event)
{
    // This is the most reliable place to detect DPI changes. QWindow::screenChanged()
    // doesn't detect when the DPI is changed on the current monitor, for example.
    handleScaleAdjustmentChange();

    QGraphicsView::paintEvent(event);
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
        scrollHelper->move(QPointF(mouseDelta.x() * (isRightToLeft() ? 1 : -1), mouseDelta.y() * -1));
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
//                centerImage();
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
        const qreal scrollDivisor = 2.0; // To make scrolling less sensitive
        qreal scrollX = event->angleDelta().x() * (isRightToLeft() ? 1 : -1) / scrollDivisor;
        qreal scrollY = event->angleDelta().y() * -1 / scrollDivisor;

        if (event->modifiers() & Qt::ShiftModifier)
            std::swap(scrollX, scrollY);

        scrollHelper->move(QPointF(scrollX, scrollY));
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
    imageCore.loadFile(fileName);
}

void QVGraphicsView::postLoad()
{
    scrollHelper->cancelAnimation();

    // Set the pixmap to the new image and reset the transform's scale to a known value
    makeUnscaled();

    if (isNavigationResetsZoomEnabled && !isZoomToFitEnabled)
        setZoomToFitEnabled(true);
    else
        fitOrConstrainImage();

    expensiveScaleTimer->start();

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

    if (!isApplyingZoomToFit)
        setZoomToFitEnabled(false);

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
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + (move.x() * (isRightToLeft() ? -1 : 1)));
        verticalScrollBar()->setValue(verticalScrollBar()->value() + move.y());
        lastZoomRoundingError = mapToScene(pos) - scenePos;
        constrainBoundsTimer->start();
    }
    else
    {
        centerImage();
    }

    expensiveScaleTimer->start();
    emitZoomLevelChangedTimer->start();
}

void QVGraphicsView::setZoomLevel(qreal absoluteScaleFactor)
{
    zoom(absoluteScaleFactor / currentScale);
}

bool QVGraphicsView::getZoomToFitEnabled() const
{
    return isZoomToFitEnabled;
}

void QVGraphicsView::setZoomToFitEnabled(bool value)
{
    if (isZoomToFitEnabled == value)
        return;

    isZoomToFitEnabled = value;
    if (isZoomToFitEnabled)
        zoomToFit();

    emit zoomToFitChanged();
}

bool QVGraphicsView::getNavigationResetsZoomEnabled() const
{
    return isNavigationResetsZoomEnabled;
}

void QVGraphicsView::setNavigationResetsZoomEnabled(bool value)
{
    if (isNavigationResetsZoomEnabled == value)
        return;

    isNavigationResetsZoomEnabled = value;

    emit navigationResetsZoomChanged();
}

void QVGraphicsView::scaleExpensively()
{
    if (!isScalingEnabled || !getCurrentFileDetails().isPixmapLoaded)
        return;

    // If we are above maximum scaling size
    const QSize contentSize = getContentRect().size().toSize();
    const QSize maxSize = getUsableViewportRect(true).size() * (isScalingTwoEnabled ? 3 : 1) + QSize(1, 1);
    if (contentSize.width() > maxSize.width() || contentSize.height() > maxSize.height())
    {
        // Return to original size
        makeUnscaled();
        return;
    }

    // Calculate scaled resolution
    qreal scaleAdjustment = getScaleAdjustment();
    const QSizeF mappedSize = QSizeF(getCurrentFileDetails().loadedPixmapSize) * currentScale * scaleAdjustment * devicePixelRatioF();

    // Set image to scaled version
    loadedPixmapItem->setPixmap(imageCore.scaleExpensively(mappedSize));

    // Set appropriate scale factor
    qreal targetScale = 1.0 / devicePixelRatioF();
    setTransform(getTransformWithNoScaling().scale(targetScale, targetScale));
    appliedScaleAdjustment = scaleAdjustment;
}

void QVGraphicsView::makeUnscaled()
{
    // Return to original size
    if (getCurrentFileDetails().isMovieLoaded)
        loadedPixmapItem->setPixmap(getLoadedMovie().currentPixmap());
    else if (getCurrentFileDetails().isPixmapLoaded)
        loadedPixmapItem->setPixmap(getLoadedPixmap());

    // Set appropriate scale factor
    qreal scaleAdjustment = getScaleAdjustment();
    qreal targetScale = currentScale * scaleAdjustment;
    setTransform(getTransformWithNoScaling().scale(targetScale, targetScale));
    appliedScaleAdjustment = scaleAdjustment;
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

void QVGraphicsView::zoomToFit()
{
    if (!getCurrentFileDetails().isPixmapLoaded)
        return;

    QSizeF effectiveImageSize = getEffectiveOriginalSize();
    QSizeF viewSize = getUsableViewportRect(true).size();

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

    isApplyingZoomToFit = true;
    setZoomLevel(targetRatio);
    isApplyingZoomToFit = false;
}

void QVGraphicsView::originalSize()
{
    setZoomLevel(1.0);
}

void QVGraphicsView::centerImage()
{
    centerOn(loadedPixmapItem);
}

void QVGraphicsView::goToFile(const GoToFileMode &mode, int index)
{
    bool shouldRetryFolderInfoUpdate = false;

    // Update folder info only after a little idle time as an optimization for when
    // the user is rapidly navigating through files.
    if (!getCurrentFileDetails().timeSinceLoaded.isValid() || getCurrentFileDetails().timeSinceLoaded.hasExpired(3000))
    {
        // Make sure the file still exists because if it disappears from the file listing we'll lose
        // track of our index within the folder. Use the static 'exists' method to avoid caching.
        // If we skip updating now, flag it for retry later once we locate a new file.
        if (QFile::exists(getCurrentFileDetails().fileInfo.absoluteFilePath()))
            imageCore.updateFolderInfo();
        else
            shouldRetryFolderInfoUpdate = true;
    }

    const auto &fileList = getCurrentFileDetails().folderFileInfoList;
    if (fileList.isEmpty())
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
                newIndex = fileList.size()-1;
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
        if (fileList.size()-1 == newIndex)
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
        newIndex = fileList.size()-1;
        searchDirection = -1;
        break;
    }
    }

    while (searchDirection == 1 && newIndex < fileList.size()-1 && !QFile::exists(fileList.value(newIndex).absoluteFilePath))
        newIndex++;
    while (searchDirection == -1 && newIndex > 0 && !QFile::exists(fileList.value(newIndex).absoluteFilePath))
        newIndex--;

    const QString nextImageFilePath = fileList.value(newIndex).absoluteFilePath;

    if (!QFile::exists(nextImageFilePath) || nextImageFilePath == getCurrentFileDetails().fileInfo.absoluteFilePath())
        return;

    if (shouldRetryFolderInfoUpdate)
    {
        // If the user just deleted a file through qView, closeImage will have been called which empties
        // currentFileDetails.fileInfo. In this case updateFolderInfo can't infer the directory from
        // fileInfo like it normally does, so we'll explicity pass in the folder here.
        imageCore.updateFolderInfo(QFileInfo(nextImageFilePath).path());
    }

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
        horizontal -= int(viewPoint.x() - (targetRect.width() / 2.0));
        horizontalScrollBar()->setValue(horizontal);
    }
    else
    {
        horizontalScrollBar()->setValue(int(viewPoint.x() - (targetRect.width() / 2.0)));
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

void QVGraphicsView::fitOrConstrainImage()
{
    if (isZoomToFitEnabled)
        zoomToFit();
    else
        scrollHelper->constrain(true);
}

QSizeF QVGraphicsView::getEffectiveOriginalSize() const
{
    return getTransformWithNoScaling().mapRect(QRectF(QPoint(), getCurrentFileDetails().loadedPixmapSize)).size() * getScaleAdjustment();
}

QRectF QVGraphicsView::getContentRect() const
{
    return transform().mapRect(loadedPixmapItem->boundingRect());
}

QRect QVGraphicsView::getUsableViewportRect(bool addMargin) const
{
#ifdef COCOA_LOADED
    int obscuredHeight = QVCocoaFunctions::getObscuredHeight(window()->windowHandle());
#else
    int obscuredHeight = 0;
#endif
    QRect rect = viewport()->rect();
    rect.setTop(obscuredHeight);
    if (addMargin)
        rect.adjust(MARGIN, MARGIN, -MARGIN, -MARGIN);
    return rect;
}

QTransform QVGraphicsView::getTransformWithNoScaling() const
{
    qreal currentTransformScale = transform().mapRect(QRectF(QPointF(), QSizeF(1, 1))).width();
    return QTransform(transform()).scale(1.0 / currentTransformScale, 1.0 / currentTransformScale);
}

qreal QVGraphicsView::getScaleAdjustment() const
{
    return isOneToOnePixelSizingEnabled ? 1.0 / devicePixelRatioF() : 1.0;
}

void QVGraphicsView::handleScaleAdjustmentChange()
{
    if (appliedScaleAdjustment == getScaleAdjustment())
        return;

    makeUnscaled();

    fitOrConstrainImage();

    expensiveScaleTimer->start();
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
    if (isScalingEnabled)
        expensiveScaleTimer->start();
    else
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

    //one-to-one pixel sizing
    isOneToOnePixelSizingEnabled = settingsManager.getBoolean("onetoonepixelsizing");

    //constrained positioning
    isConstrainedPositioningEnabled = settingsManager.getBoolean("constrainimageposition");

    //constrained small centering
    isConstrainedSmallCenteringEnabled = settingsManager.getBoolean("constraincentersmallimage");

    //loop folders
    isLoopFoldersEnabled = settingsManager.getBoolean("loopfoldersenabled");

    // End of settings variables

    handleScaleAdjustmentChange();

    fitOrConstrainImage();
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
