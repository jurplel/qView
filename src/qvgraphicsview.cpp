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
    isConstrainedPositioningEnabled = false;
    isConstrainedSmallCenteringEnabled = true;
    cropMode = 0;
    zoomMultiplier = 1.25;

    // Initialize other variables
    fitOverscan = 0;
    resizeResetsZoom = true;
    navResetsZoom = true;
    zoomLevel = 1.0;
    appliedDpiAdjustment = 1.0;
    appliedExpensiveScaleZoomLevel = 0.0;
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
    connect(expensiveScaleTimer, &QTimer::timeout, this, [this]{applyExpensiveScaling();});

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

    if (resizeResetsZoom)
        zoomToFit();
    else
        scrollHelper->constrain(true);
}

void QVGraphicsView::paintEvent(QPaintEvent *event)
{
    // This is the most reliable place to detect DPI changes. QWindow::screenChanged()
    // doesn't detect when the DPI is changed on the current monitor, for example.
    handleDpiAdjustmentChange();

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
                zoomRelative(pinchGesture->scaleFactor(), hotPoint);
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
    const qreal zoomAmountPerWheelClick = zoomMultiplier - 1.0;
    qreal zoomFactor = 1.0 + (fractionalWheelClicks * zoomAmountPerWheelClick);

    if (yDelta < 0)
        zoomFactor = qPow(zoomFactor, -1);

    zoomRelative(zoomFactor, eventPos);
}

void QVGraphicsView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down || event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
    {
        // Normally the arrow keys are assigned to shortcuts, but in case they aren't or
        // get passed through due to modifier keys, handle it here instead of letting the
        // base class do it to ensure any bounds constraints are enforced.
        const int stepDown = verticalScrollBar()->singleStep();
        const int stepRight = horizontalScrollBar()->singleStep() * (isRightToLeft() ? -1 : 1);
        QPoint delta {};
        if (event->key() == Qt::Key_Up) delta.ry() -= stepDown;
        if (event->key() == Qt::Key_Down) delta.ry() += stepDown;
        if (event->key() == Qt::Key_Left) delta.rx() -= stepRight;
        if (event->key() == Qt::Key_Right) delta.rx() += stepRight;
        scrollHelper->move(delta);
        constrainBoundsTimer->start();
        return;
    }

    QGraphicsView::keyPressEvent(event);
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

void QVGraphicsView::reloadFile()
{
    if (!getCurrentFileDetails().isPixmapLoaded)
        return;

    imageCore.loadFile(getCurrentFileDetails().fileInfo.absoluteFilePath(), true);
}

void QVGraphicsView::postLoad()
{
    scrollHelper->cancelAnimation();

    // Set the pixmap to the new image and reset the transform's scale to a known value
    removeExpensiveScaling();

    if (navResetsZoom)
        zoomToFit();
    else
        scrollHelper->constrain(true);

    expensiveScaleTimer->start();

    qvApp->getActionManager().addFileToRecentsList(getCurrentFileDetails().fileInfo);

    emit fileChanged();
}

void QVGraphicsView::zoomIn(const QPoint &pos)
{
    zoomRelative(zoomMultiplier, pos);
}

void QVGraphicsView::zoomOut(const QPoint &pos)
{
    zoomRelative(qPow(zoomMultiplier, -1), pos);
}

void QVGraphicsView::zoomRelative(qreal relativeLevel, const QPoint &pos)
{
    const qreal absoluteLevel = zoomLevel * relativeLevel;

    if (absoluteLevel >= 500 || absoluteLevel <= 0.01)
        return;

    zoomAbsolute(absoluteLevel, pos);
}

void QVGraphicsView::zoomAbsolute(const qreal absoluteLevel, const QPoint &pos)
{
    if (pos != lastZoomEventPos)
    {
        lastZoomEventPos = pos;
        lastZoomRoundingError = QPointF();
    }
    const QPointF scenePos = mapToScene(pos) - lastZoomRoundingError;

    if (appliedExpensiveScaleZoomLevel != 0.0)
    {
        const qreal baseTransformScale = 1.0 / devicePixelRatioF();
        const qreal relativeLevel = absoluteLevel / appliedExpensiveScaleZoomLevel;
        setTransformScale(baseTransformScale * relativeLevel);
    }
    else
    {
        setTransformScale(absoluteLevel * appliedDpiAdjustment);
    }
    zoomLevel = absoluteLevel;

    scrollHelper->cancelAnimation();

    // If we have a point to zoom towards and cursor zooming is enabled/applicable
    if (pos != QPoint(-1, -1) && isCursorZoomEnabled && (isConstrainedPositioningEnabled || getContentToViewportRatio() >= 1.0))
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

bool QVGraphicsView::getResizeResetsZoom() const
{
    return resizeResetsZoom;
}

void QVGraphicsView::setResizeResetsZoom(bool value)
{
    if (resizeResetsZoom == value)
        return;

    resizeResetsZoom = value;

    emit resizeResetsZoomChanged();
}

bool QVGraphicsView::getNavResetsZoom() const
{
    return navResetsZoom;
}

void QVGraphicsView::setNavResetsZoom(bool value)
{
    if (navResetsZoom == value)
        return;

    navResetsZoom = value;

    emit navResetsZoomChanged();
}

void QVGraphicsView::applyExpensiveScaling()
{
    if (!isScalingEnabled || !getCurrentFileDetails().isPixmapLoaded)
        return;

    // If we are above maximum scaling size
    if (getContentToViewportRatio() > (isScalingTwoEnabled ? 3.0 : 1.00001))
    {
        // Return to original size
        removeExpensiveScaling();
        return;
    }

    // Calculate scaled resolution
    const qreal dpiAdjustment = getDpiAdjustment();
    const QSizeF mappedSize = QSizeF(getCurrentFileDetails().loadedPixmapSize) * zoomLevel * dpiAdjustment * devicePixelRatioF();

    // Set image to scaled version
    loadedPixmapItem->setPixmap(imageCore.scaleExpensively(mappedSize));

    // Set appropriate scale factor
    const qreal newTransformScale = 1.0 / devicePixelRatioF();
    setTransformScale(newTransformScale);
    appliedDpiAdjustment = dpiAdjustment;
    appliedExpensiveScaleZoomLevel = zoomLevel;
}

void QVGraphicsView::removeExpensiveScaling()
{
    // Return to original size
    if (getCurrentFileDetails().isMovieLoaded)
        loadedPixmapItem->setPixmap(getLoadedMovie().currentPixmap());
    else if (getCurrentFileDetails().isPixmapLoaded)
        loadedPixmapItem->setPixmap(getLoadedPixmap());

    // Set appropriate scale factor
    const qreal dpiAdjustment = getDpiAdjustment();
    const qreal newTransformScale = zoomLevel * dpiAdjustment;
    setTransformScale(newTransformScale);
    appliedDpiAdjustment = dpiAdjustment;
    appliedExpensiveScaleZoomLevel = 0.0;
}

void QVGraphicsView::animatedFrameChanged(QRect rect)
{
    Q_UNUSED(rect)

    if (isScalingEnabled)
    {
        applyExpensiveScaling();
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
    QSize viewSize = getUsableViewportRect(true).size();

    if (viewSize.isEmpty())
        return;

    qreal fitXRatio = viewSize.width() / effectiveImageSize.width();
    qreal fitYRatio = viewSize.height() / effectiveImageSize.height();

    qreal targetRatio;

    // Each mode will check if the rounded image size already produces the desired fit,
    // in which case we can use exactly 1.0 to avoid unnecessary scaling

    switch (cropMode) { // should be enum tbh
    case 1: // only take into account height
        if (qRound(effectiveImageSize.height()) == viewSize.height())
            targetRatio = 1.0;
        else
            targetRatio = fitYRatio;
        break;
    case 2: // only take into account width
        if (qRound(effectiveImageSize.width()) == viewSize.width())
            targetRatio = 1.0;
        else
            targetRatio = fitXRatio;
        break;
    default:
        if ((qRound(effectiveImageSize.height()) == viewSize.height() && qRound(effectiveImageSize.width()) <= viewSize.width()) ||
            (qRound(effectiveImageSize.width()) == viewSize.width() && qRound(effectiveImageSize.height()) <= viewSize.height()))
        {
            targetRatio = 1.0;
        }
        else
        {
            QSize xRatioSize = (effectiveImageSize * fitXRatio * devicePixelRatioF()).toSize();
            QSize yRatioSize = (effectiveImageSize * fitYRatio * devicePixelRatioF()).toSize();
            QSize maxSize = (QSizeF(viewSize) * devicePixelRatioF()).toSize();
            // If the fit ratios are extremely close, it's possible that both are sufficient to
            // contain the image, but one results in the opposing dimension getting rounded down
            // to just under the view size, so use the larger of the two ratios in that case.
            if (xRatioSize.boundedTo(maxSize) == xRatioSize && yRatioSize.boundedTo(maxSize) == yRatioSize)
                targetRatio = qMax(fitXRatio, fitYRatio);
            else
                targetRatio = qMin(fitXRatio, fitYRatio);
        }
        break;
    }

    if (targetRatio > 1.0 && !isPastActualSizeEnabled)
        targetRatio = 1.0;

    zoomAbsolute(targetRatio);
}

void QVGraphicsView::originalSize()
{
    zoomAbsolute(1.0);
}

void QVGraphicsView::centerImage()
{
    const QRect viewRect = getUsableViewportRect();
    const QRect contentRect = getContentRect().toRect();
    const int hOffset = isRightToLeft() ?
        horizontalScrollBar()->minimum() + horizontalScrollBar()->maximum() - contentRect.left() :
        contentRect.left();
    const int vOffset = contentRect.top() - viewRect.top();
    const int hOverflow = contentRect.width() - viewRect.width();
    const int vOverflow = contentRect.height() - viewRect.height();

    horizontalScrollBar()->setValue(hOffset + (hOverflow / (isRightToLeft() ? -2 : 2)));
    verticalScrollBar()->setValue(vOffset + (vOverflow / 2));

    scrollHelper->cancelAnimation();
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

    if (searchDirection != 0)
    {
        while (searchDirection == 1 && newIndex < fileList.size()-1 && !QFile::exists(fileList.value(newIndex).absoluteFilePath))
            newIndex++;
        while (searchDirection == -1 && newIndex > 0 && !QFile::exists(fileList.value(newIndex).absoluteFilePath))
            newIndex--;
    }

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

QSizeF QVGraphicsView::getEffectiveOriginalSize() const
{
    return getTransformWithNoScaling().mapRect(QRectF(QPoint(), getCurrentFileDetails().loadedPixmapSize)).size() * getDpiAdjustment();
}

QRectF QVGraphicsView::getContentRect() const
{
    return transform().mapRect(loadedPixmapItem->boundingRect());
}

QRect QVGraphicsView::getUsableViewportRect(bool addOverscan) const
{
#ifdef COCOA_LOADED
    int obscuredHeight = QVCocoaFunctions::getObscuredHeight(window()->windowHandle());
#else
    int obscuredHeight = 0;
#endif
    QRect rect = viewport()->rect();
    rect.setTop(obscuredHeight);
    if (addOverscan)
        rect.adjust(-fitOverscan, -fitOverscan, fitOverscan, fitOverscan);
    return rect;
}

qreal QVGraphicsView::getContentToViewportRatio() const
{
    const QSizeF contentSize = getContentRect().size();
    const QSizeF viewportSize = getUsableViewportRect(true).size();
    return qMax(contentSize.width() / viewportSize.width(), contentSize.height() / viewportSize.height());
}

void QVGraphicsView::setTransformScale(qreal value)
{
#ifdef Q_OS_WIN
    // On Windows, the positioning of scaled pixels seems to follow a floor rule rather
    // than rounding, so increase the scale just a hair to cover rounding errors in case
    // the desired scale was targeting an integer pixel boundary.
    value *= 1.0 + std::numeric_limits<double>::epsilon();
#endif
    setTransform(getTransformWithNoScaling().scale(value, value));
}

QTransform QVGraphicsView::getTransformWithNoScaling() const
{
    const QTransform t = transform();
    // Only intended to handle combinations of scaling, mirroring, flipping, and rotation
    // in increments of 90 degrees. A seemingly simpler approach would be to scale the
    // transform by the inverse of its scale factor, but the resulting scale factor may
    // not exactly equal 1 due to floating point rounding errors.
    if (t.type() == QTransform::TxRotate)
        return { 0, t.m12() < 0 ? -1.0 : 1.0, t.m21() < 0 ? -1.0 : 1.0, 0, 0, 0 };
    else
        return { t.m11() < 0 ? -1.0 : 1.0, 0, 0, t.m22() < 0 ? -1.0 : 1.0, 0, 0 };
}

qreal QVGraphicsView::getDpiAdjustment() const
{
    return isOneToOnePixelSizingEnabled ? 1.0 / devicePixelRatioF() : 1.0;
}

void QVGraphicsView::handleDpiAdjustmentChange()
{
    if (appliedDpiAdjustment == getDpiAdjustment())
        return;

    removeExpensiveScaling();

    if (resizeResetsZoom)
        zoomToFit();
    else
        scrollHelper->constrain(true);

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
        removeExpensiveScaling();

    //scaling2
    if (!isScalingEnabled)
        isScalingTwoEnabled = false;
    else
        isScalingTwoEnabled = settingsManager.getBoolean("scalingtwoenabled");

    //cropmode
    cropMode = settingsManager.getInteger("cropmode");

    //scalefactor
    zoomMultiplier = 1.0 + (settingsManager.getInteger("scalefactor") / 100.0);

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

    handleDpiAdjustmentChange();

    if (resizeResetsZoom)
        zoomToFit();
    else
        scrollHelper->constrain(true);
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
