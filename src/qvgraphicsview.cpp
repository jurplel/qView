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
#include "qvgraphicspixmapitem.h"

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
    cropMode = 0;
    scaleFactor = 1.25;

    // Initialize other variables
    currentScale = 1.0;
    scaledSize = QSize();
    maxScalingTwoSize = 3;
    isOriginalSize = false;

    zoomBasisScaleFactor = 1.0;

    connect(&imageCore, &QVImageCore::animatedFrameChanged, this, &QVGraphicsView::animatedFrameChanged);
    connect(&imageCore, &QVImageCore::fileChanged, this, &QVGraphicsView::postLoad);
    connect(&imageCore, &QVImageCore::updateLoadedPixmapItem, this, &QVGraphicsView::updateLoadedPixmapItem);
    connect(&imageCore, &QVImageCore::readError, this, &QVGraphicsView::error);

    // Should replace the other timer eventually
    expensiveScaleTimerNew = new QTimer(this);
    expensiveScaleTimerNew->setSingleShot(true);
    expensiveScaleTimerNew->setInterval(50);
    connect(expensiveScaleTimerNew, &QTimer::timeout, this, [this]{scaleExpensively();});

    loadedPixmapItem = new QVGraphicsPixmapItem();
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
        if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) || event->modifiers() == Qt::ShiftModifier)
            translate(event->angleDelta().y()/2.0, event->angleDelta().x()/2.0);
        else
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
    updateLoadedPixmapItem();
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

    const QPointF scenePos = mapToScene(pos);

    zoomBasisScaleFactor *= scaleFactor;
    setTransform(QTransform(zoomBasis).scale(zoomBasisScaleFactor, zoomBasisScaleFactor));
    absoluteTransform.scale(scaleFactor, scaleFactor);

    // If we are zooming in, we have a point to zoom towards, the mouse is on top of the viewport, and cursor zooming is enabled
    if (currentScale > 1.00001 && pos != QPoint(-1, -1) && underMouse() && isCursorZoomEnabled)
    {
        const QPointF p1mouse = mapFromScene(scenePos);
        const QPointF move = p1mouse - pos;
        horizontalScrollBar()->setValue(move.x() + horizontalScrollBar()->value());
        verticalScrollBar()->setValue(move.y() + verticalScrollBar()->value());
    }
    else
    {
        centerOn(loadedPixmapItem);
    }

    if (isScalingEnabled && !isOriginalSize)
    {
        expensiveScaleTimerNew->start();
    }
}

void QVGraphicsView::scaleExpensively()
{
    // Determine if mirrored or flipped
    bool mirrored = false;
    if (transform().m11() < 0)
        mirrored = true;

    bool flipped = false;
    if (transform().m22() < 0)
        flipped = true;

    // If we are above maximum scaling size
    if ((currentScale >= maxScalingTwoSize) ||
        (!isScalingTwoEnabled && currentScale > 1.00001))
    {
        // Return to original size
        makeUnscaled();
        return;
    }

    // Map size of the original pixmap to the scale acquired in fitting with modification from zooming percentage
    const QRectF mappedRect = absoluteTransform.mapRect(QRectF({}, getCurrentFileDetails().loadedPixmapSize));
    const QSizeF mappedPixmapSize = mappedRect.size() * devicePixelRatioF();

    // Undo mirror/flip before new transform
    if (mirrored)
        scale(-1, 1);

    if (flipped)
        scale(1, -1);

    // Set image to scaled version
    loadedPixmapItem->setPixmap(imageCore.scaleExpensively(mappedPixmapSize));

    // Reset transformation
    setTransform(QTransform::fromScale(qPow(devicePixelRatioF(), -1), qPow(devicePixelRatioF(), -1)));

    // Redo mirror/flip after new transform
    if (mirrored)
        scale(-1, 1);

    if (flipped)
        scale(1, -1);

    // Set zoombasis
    zoomBasis = transform();
    zoomBasisScaleFactor = 1.0;
}

void QVGraphicsView::makeUnscaled()
{
    // Determine if mirrored or flipped
    bool mirrored = false;
    if (transform().m11() < 0)
        mirrored = true;

    bool flipped = false;
    if (transform().m22() < 0)
        flipped = true;

    // Return to original size
    if (getCurrentFileDetails().isMovieLoaded)
        loadedPixmapItem->setPixmap(getLoadedMovie().currentPixmap());
    else
        loadedPixmapItem->setPixmap(getLoadedPixmap());

    setTransform(absoluteTransform);

    // Redo mirror/flip after new transform
    if (mirrored)
        scale(-1, 1);

    if (flipped)
        scale(1, -1);

    // Reset transformation
    zoomBasis = transform();
    zoomBasisScaleFactor = 1.0;
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

void QVGraphicsView::updateLoadedPixmapItem()
{
    //set pixmap and offset
    loadedPixmapItem->setPixmap(getLoadedPixmap());
    scaledSize = loadedPixmapItem->boundingRect().size().toSize();

    resetScale();

    emit updatedLoadedPixmapItem();
}

void QVGraphicsView::resetScale()
{
    if (!getCurrentFileDetails().isPixmapLoaded)
        return;

    fitInViewMarginless(loadedPixmapItem);

    if (isScalingEnabled)
        expensiveScaleTimerNew->start();
}

void QVGraphicsView::originalSize()
{
    if (isOriginalSize)
    {
        // If we are at the actual original size
        if (transform() == QTransform())
        {
            resetScale(); // back to normal mode
            return;
        }
    }
    makeUnscaled();

    resetTransform();
    centerOn(loadedPixmapItem);

    zoomBasis = transform();
    zoomBasisScaleFactor = 1.0;
    absoluteTransform = transform();

    isOriginalSize = true;
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

void QVGraphicsView::fitInViewMarginless(const QRectF &rect)
{
#ifdef COCOA_LOADED
    int obscuredHeight = QVCocoaFunctions::getObscuredHeight(window()->windowHandle());
#else
    int obscuredHeight = 0;
#endif

    // Set adjusted image size / bounding rect based on
    QSize adjustedImageSize = getCurrentFileDetails().loadedPixmapSize;
    QRectF adjustedBoundingRect = rect;

    switch (cropMode) { // should be enum tbh
    case 1: // only take into account height
    {
        adjustedImageSize.setWidth(1);
        adjustedBoundingRect.setWidth(1);
        break;
    }
    case 2: // only take into account width
    {
        adjustedImageSize.setHeight(1);
        adjustedBoundingRect.setHeight(1);
        break;
    }
    }
    adjustedBoundingRect.moveCenter(rect.center());

    if (!scene() || adjustedBoundingRect.isNull())
        return;

    // Reset the view scale to 1:1.
    QRectF unity = transform().mapRect(QRectF(0, 0, 1, 1));
    if (unity.isEmpty())
        return;
    scale(1 / unity.width(), 1 / unity.height());

    // Determine what we are resizing to
    const int adjWidth = width() - MARGIN;
    const int adjHeight = height() - MARGIN - obscuredHeight;

    QRectF viewRect;
    // Resize to window size unless you are meant to stop at the actual size, basically
    if (isPastActualSizeEnabled || (adjustedImageSize.width() >= adjWidth || adjustedImageSize.height() >= adjHeight))
    {
        viewRect = viewport()->rect().adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);
        viewRect.setHeight(viewRect.height() - obscuredHeight);
    }
    else
    {
        // stop at actual size
        viewRect = QRect(QPoint(), getCurrentFileDetails().loadedPixmapSize);
        QPoint center = this->rect().center();
        center.setY(center.y() - obscuredHeight);
        viewRect.moveCenter(center);
    }


    if (viewRect.isEmpty())
        return;

    // Find the ideal x / y scaling ratio to fit \a rect in the view.
    QRectF sceneRect = transform().mapRect(adjustedBoundingRect);
    if (sceneRect.isEmpty())
        return;

    qreal xratio = viewRect.width() / sceneRect.width();
    qreal yratio = viewRect.height() / sceneRect.height();

    xratio = yratio = qMin(xratio, yratio);

    // Find and set the transform required to fit the original image
    // Compact version of above code
    QRectF sceneRect2 = transform().mapRect(QRectF({}, adjustedImageSize));
    qreal absoluteRatio = qMin(viewRect.width() / sceneRect2.width(),
                               viewRect.height() / sceneRect2.height());

    absoluteTransform = QTransform::fromScale(absoluteRatio, absoluteRatio);

    // Scale and center on the center of \a rect.
    scale(xratio, yratio);
    centerOn(adjustedBoundingRect.center());

    // variables
    zoomBasis = transform();

    isOriginalSize = false;
    currentScale = 1.0;
}

void QVGraphicsView::fitInViewMarginless(const QGraphicsItem *item)
{
    return fitInViewMarginless(item->sceneBoundingRect());
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
    centerOn(item->sceneBoundingRect().center());
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

    loadedPixmapItem->setDrawCheckerBoardBackground(settingsManager.getBoolean("checkerboardBackground"));

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
    imageCore.rotateImage(rotation);
}
