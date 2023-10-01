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
    viewport()->setAutoFillBackground(false);
    grabGesture(Qt::PinchGesture);

    // Scene setup
    auto *scene = new QGraphicsScene(-1000000.0, -1000000.0, 2000000.0, 2000000.0, this);
    setScene(scene);

    // Initialize configurable variables
    isScalingEnabled = true;
    isScalingTwoEnabled = true;
    isPastActualSizeEnabled = true;
    fitOverscan = 0;
    isScrollZoomsEnabled = true;
    isLoopFoldersEnabled = true;
    isCursorZoomEnabled = true;
    isOneToOnePixelSizingEnabled = true;
    isConstrainedPositioningEnabled = true;
    isConstrainedSmallCenteringEnabled = true;
    sidewaysScrollNavigates = false;
    cropMode = Qv::FitMode::WholeImage;
    zoomMultiplier = 1.25;

    // Initialize other variables
    isZoomToFitEnabled = true;
    isApplyingZoomToFit = false;
    isNavigationResetsZoomEnabled = true;
    loadIsFromSessionRestore = false;
    zoomLevel = 1.0;
    appliedDpiAdjustment = 1.0;
    appliedExpensiveScaleZoomLevel = 0.0;
    lastZoomEventPos = QPoint(-1, -1);
    lastZoomRoundingError = QPointF();

    scrollHelper = new ScrollHelper(this,
        [this](ScrollHelper::Parameters &p)
        {
            p.contentRect = getContentRect().toRect();
            p.usableViewportRect = getUsableViewportRect();
            p.shouldConstrain = isConstrainedPositioningEnabled;
            p.shouldCenter = isConstrainedSmallCenteringEnabled;
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
    if (const auto mainWindow = getMainWindow())
        if (mainWindow->getIsClosing())
            return;

    QGraphicsView::resizeEvent(event);
    fitOrConstrainImage();
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
        const bool isAltAction = event->modifiers().testFlag(Qt::ControlModifier);
        if ((isAltAction ? altDragAction : dragAction) != Qv::ViewportDragAction::None)
        {
            pressedMouseButton = Qt::LeftButton;
            mousePressModifiers = event->modifiers();
            viewport()->setCursor(Qt::ClosedHandCursor);
            lastMousePos = event->pos();
        }
        return;
    }
    else if (event->button() == Qt::MouseButton::MiddleButton)
    {
        const bool isAltAction = event->modifiers().testFlag(Qt::ControlModifier);
        executeClickAction(isAltAction ? altMiddleClickAction : middleClickAction);
        return;
    }
    else if (event->button() == Qt::MouseButton::BackButton)
    {
        goToFile(GoToFileMode::previous);
        return;
    }
    else if (event->button() == Qt::MouseButton::ForwardButton)
    {
        goToFile(GoToFileMode::next);
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void QVGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (pressedMouseButton == Qt::LeftButton)
    {
        pressedMouseButton = Qt::NoButton;
        mousePressModifiers = Qt::NoModifier;
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
        const bool isAltAction = mousePressModifiers.testFlag(Qt::ControlModifier);
        const QPoint delta = event->pos() - lastMousePos;
        bool isMovingWindow = false;
        executeDragAction(isAltAction ? altDragAction : dragAction, delta, isMovingWindow);
        if (!isMovingWindow)
            lastMousePos = event->pos();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void QVGraphicsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        const bool isAltAction = event->modifiers().testFlag(Qt::ControlModifier);
        executeClickAction(isAltAction ? altDoubleClickAction : doubleClickAction);
        return;
    }

    QGraphicsView::mouseDoubleClickEvent(event);
}

bool QVGraphicsView::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
    {
        QGestureEvent *gestureEvent = static_cast<QGestureEvent*>(event);
        if (QGesture *pinch = gestureEvent->gesture(Qt::PinchGesture))
        {
            QPinchGesture *pinchGesture = static_cast<QPinchGesture*>(pinch);
            if (pinchGesture->changeFlags() & QPinchGesture::ScaleFactorChanged)
            {
                const QPoint hotPoint = mapFromGlobal(pinchGesture->hotSpot().toPoint());
                zoomRelative(pinchGesture->scaleFactor(), hotPoint);
            }
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
    const bool isAltAction = event->modifiers().testFlag(Qt::ControlModifier);
    const Qv::ViewportScrollAction horizontalAction = isAltAction ? altHorizontalScrollAction : horizontalScrollAction;
    const Qv::ViewportScrollAction verticalAction = isAltAction ? altVerticalScrollAction : verticalScrollAction;
    const bool hasHorizontalAction = horizontalAction != Qv::ViewportScrollAction::None;
    const bool hasVerticalAction = verticalAction != Qv::ViewportScrollAction::None;
    if (!hasHorizontalAction && !hasVerticalAction)
        return;
    const QPoint baseDelta =
        hasHorizontalAction && !hasVerticalAction ? QPoint(event->angleDelta().x(), 0) :
        !hasHorizontalAction && hasVerticalAction ? QPoint(0, event->angleDelta().y()) :
        event->angleDelta();
    const QPoint effectiveDelta =
        horizontalAction == verticalAction ? baseDelta :
        scrollAxisLocker.filterMovement(baseDelta, event->phase(), hasHorizontalAction != hasVerticalAction);
    const Qv::ViewportScrollAction effectiveAction =
        effectiveDelta.x() != 0 ? horizontalAction :
        effectiveDelta.y() != 0 ? verticalAction :
        Qv::ViewportScrollAction::None;
    if (effectiveAction == Qv::ViewportScrollAction::None)
        return;
    const bool hasShiftModifier = event->modifiers().testFlag(Qt::ShiftModifier);

    executeScrollAction(effectiveAction, effectiveDelta, eventPos, hasShiftModifier);
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

void QVGraphicsView::executeClickAction(const Qv::ViewportClickAction action)
{
    if (action == Qv::ViewportClickAction::ZoomToFit)
    {
        if (!getZoomToFitEnabled())
            setZoomToFitEnabled(true);
        else
            centerImage();
    }
    else if (action == Qv::ViewportClickAction::OriginalSize)
    {
        originalSize();
    }
    else if (action == Qv::ViewportClickAction::ToggleFullScreen)
    {
        if (const auto mainWindow = getMainWindow())
            mainWindow->toggleFullScreen();
    }
    else if (action == Qv::ViewportClickAction::ToggleTitlebarHidden)
    {
        if (const auto mainWindow = getMainWindow())
            mainWindow->toggleTitlebarHidden();
    }
}

void QVGraphicsView::executeDragAction(const Qv::ViewportDragAction action, const QPoint delta, bool &isMovingWindow)
{
    if (action == Qv::ViewportDragAction::Pan)
    {
        scrollHelper->move(QPointF(-delta.x() * (isRightToLeft() ? -1 : 1), -delta.y()));
    }
    else if (action == Qv::ViewportDragAction::MoveWindow)
    {
        window()->move(window()->pos() + delta);
        isMovingWindow = true;
    }
}

void QVGraphicsView::executeScrollAction(const Qv::ViewportScrollAction action, const QPoint delta, const QPoint mousePos, const bool hasShiftModifier)
{
    const int deltaPerWheelStep = 120;
    const int rtlFlip = isRightToLeft() ? -1 : 1;

    const auto getUniAxisDelta = [delta, rtlFlip]() {
        return
            delta.x() != 0 && delta.y() == 0 ? delta.x() * rtlFlip :
            delta.x() == 0 && delta.y() != 0 ? delta.y() :
            0;
    };

    if (action == Qv::ViewportScrollAction::Pan)
    {
        const qreal scrollDivisor = 2.0; // To make scrolling less sensitive
        qreal scrollX = -delta.x() * rtlFlip / scrollDivisor;
        qreal scrollY = -delta.y() / scrollDivisor;

        if (hasShiftModifier)
            std::swap(scrollX, scrollY);

        scrollHelper->move(QPointF(scrollX, scrollY));
        constrainBoundsTimer->start();
    }
    else if (action == Qv::ViewportScrollAction::Zoom)
    {
        if (!getCurrentFileDetails().isPixmapLoaded)
            return;

        const int uniAxisDelta = getUniAxisDelta();
        const qreal fractionalWheelSteps = qFabs(uniAxisDelta) / deltaPerWheelStep;
        const qreal zoomAmountPerWheelStep = zoomMultiplier - 1.0;
        qreal zoomFactor = 1.0 + (fractionalWheelSteps * zoomAmountPerWheelStep);

        if (uniAxisDelta < 0)
            zoomFactor = qPow(zoomFactor, -1);

        zoomRelative(zoomFactor, mousePos);
    }
    else if (action == Qv::ViewportScrollAction::Navigate)
    {
        SwipeData swipeData = scrollAxisLocker.getCustomData().value<SwipeData>();
        if (swipeData.triggeredAction)
            return;
        swipeData.totalDelta += getUniAxisDelta();
        if (qAbs(swipeData.totalDelta) >= deltaPerWheelStep)
        {
            if (swipeData.totalDelta < 0)
                goToFile(GoToFileMode::next);
            else
                goToFile(GoToFileMode::previous);
            swipeData.triggeredAction = true;
        }
        scrollAxisLocker.setCustomData(QVariant::fromValue(swipeData));
    }
}

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

    if (!loadIsFromSessionRestore)
    {
        if (isNavigationResetsZoomEnabled && !isZoomToFitEnabled)
            setZoomToFitEnabled(true);
        else
            fitOrConstrainImage();
    }

    expensiveScaleTimer->start();

    qvApp->getActionManager().addFileToRecentsList(getCurrentFileDetails().fileInfo);

    emit fileChanged(loadIsFromSessionRestore);

    loadIsFromSessionRestore = false;
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
    if (!isApplyingZoomToFit)
        setZoomToFitEnabled(false);

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
    else if (!loadIsFromSessionRestore)
    {
        centerImage();
    }

    expensiveScaleTimer->start();
    emitZoomLevelChangedTimer->start();
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

void QVGraphicsView::applyExpensiveScaling()
{
    if (!isScalingEnabled || !getCurrentFileDetails().isPixmapLoaded)
        return;

    // If we are above maximum scaling size
    const QSize contentSize = getContentRect().size().toSize();
    const QSize maxSize = getUsableViewportRect(true).size() * (isScalingTwoEnabled ? 3 : 1) + QSize(1, 1);
    if (contentSize.width() > maxSize.width() || contentSize.height() > maxSize.height())
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
    else
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

    switch (cropMode) {
    case Qv::FitMode::OnlyHeight:
        if (qRound(effectiveImageSize.height()) == viewSize.height())
            targetRatio = 1.0;
        else
            targetRatio = fitYRatio;
        break;
    case Qv::FitMode::OnlyWidth:
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

    isApplyingZoomToFit = true;
    zoomAbsolute(targetRatio);
    isApplyingZoomToFit = false;
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

const QJsonObject QVGraphicsView::getSessionState() const
{
    QJsonObject state;

    const QTransform transform = getTransformWithNoScaling();
    const QJsonArray transformValues {
        static_cast<int>(transform.m11()),
        static_cast<int>(transform.m22()),
        static_cast<int>(transform.m21()),
        static_cast<int>(transform.m12())
    };
    state["transform"] = transformValues;

    state["zoomLevel"] = zoomLevel;

    state["hScroll"] = horizontalScrollBar()->value();

    state["vScroll"] = verticalScrollBar()->value();

    state["navResetsZoom"] = isNavigationResetsZoomEnabled;

    state["zoomToFit"] = isZoomToFitEnabled;

    return state;
}

void QVGraphicsView::loadSessionState(const QJsonObject &state)
{
    const QJsonArray transformValues = state["transform"].toArray();
    const QTransform transform {
        static_cast<double>(transformValues.at(0).toInt()),
        static_cast<double>(transformValues.at(3).toInt()),
        static_cast<double>(transformValues.at(2).toInt()),
        static_cast<double>(transformValues.at(1).toInt()),
        0,
        0
    };
    setTransform(transform);

    zoomAbsolute(state["zoomLevel"].toDouble());

    horizontalScrollBar()->setValue(state["hScroll"].toInt());

    verticalScrollBar()->setValue(state["vScroll"].toInt());

    isNavigationResetsZoomEnabled = state["navResetsZoom"].toBool();

    isZoomToFitEnabled = state["zoomToFit"].toBool();

    emit navigationResetsZoomChanged();
    emit zoomToFitChanged();
}

void QVGraphicsView::setLoadIsFromSessionRestore(const bool value)
{
    loadIsFromSessionRestore = value;
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
    case GoToFileMode::random:
    {
        if (fileList.size() > 1)
        {
            int randomIndex = QRandomGenerator::global()->bounded(fileList.size()-1);
            newIndex = randomIndex + (randomIndex >= newIndex ? 1 : 0);
        }
        searchDirection = 1;
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

void QVGraphicsView::fitOrConstrainImage()
{
    if (isZoomToFitEnabled)
        zoomToFit();
    else
        scrollHelper->constrain(true);
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

    fitOrConstrainImage();

    expensiveScaleTimer->start();
}

MainWindow* QVGraphicsView::getMainWindow() const
{
    return qobject_cast<MainWindow*>(window());
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
    cropMode = settingsManager.getEnum<Qv::FitMode>("cropmode");

    //scalefactor
    zoomMultiplier = 1.0 + (settingsManager.getInteger("scalefactor") / 100.0);

    //resize past actual size
    isPastActualSizeEnabled = settingsManager.getBoolean("pastactualsizeenabled");

    //fit overscan
    fitOverscan = settingsManager.getInteger("fitoverscan");

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

    //sideways scroll navigates
    sidewaysScrollNavigates = settingsManager.getBoolean("sidewaysscrollnavigates");

    //loop folders
    isLoopFoldersEnabled = settingsManager.getBoolean("loopfoldersenabled");

    // End of settings variables

    horizontalScrollAction = Qv::ViewportScrollAction::Pan;
    verticalScrollAction = Qv::ViewportScrollAction::Pan;
    altHorizontalScrollAction = sidewaysScrollNavigates ? Qv::ViewportScrollAction::Navigate : Qv::ViewportScrollAction::None;
    altVerticalScrollAction = Qv::ViewportScrollAction::Zoom;
    if (isScrollZoomsEnabled)
    {
        std::swap(horizontalScrollAction, altHorizontalScrollAction);
        std::swap(verticalScrollAction, altVerticalScrollAction);
    }

    handleDpiAdjustmentChange();

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

void QVGraphicsView::rotateImage(const int relativeAngle)
{
    rotate(relativeAngle);
}

void QVGraphicsView::mirrorImage()
{
    scale(-1, 1);
}

void QVGraphicsView::flipImage()
{
    scale(1, -1);
}
