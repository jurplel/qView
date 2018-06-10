#include "qvgraphicsview.h"
#include "qvapplication.h"

#include <QDebug>
#include <QWheelEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QSettings>
#include <QMessageBox>

QVGraphicsView::QVGraphicsView(QWidget *parent) : QGraphicsView(parent)
{
    //qgraphicsscene setup
    QGraphicsScene *scene = new QGraphicsScene(0.0, 0.0, 100000.0, 100000.0, this);
    setScene(scene);

    scaleFactor = 0.25;
    isPixmapLoaded = false;
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(10);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerExpired()));

    parentMainWindow = ((MainWindow*)parentWidget()->parentWidget());

    reader.setDecideFormatFromContent(true);
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
    zoom(event->angleDelta().y());
}


// Functions

void QVGraphicsView::zoom(int DeltaY)
{
    if (!isPixmapLoaded)
        return;

    if (isOriginalSize)
    {
        isOriginalSize = false;
        resetScale();
        return;
    }

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
                resetScale();
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
            calculateBoundingBox();
            fitInViewMarginless(alternateBoundingBox, Qt::KeepAspectRatio);
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
    reader.setFileName(fileName);
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
    else if (!loadedPixmap.convertFromImage(reader.read()))
    {
        QMessageBox::critical(this, tr("qView Error"), tr("Error: Failed to load file"));
        if (getIsPixmapLoaded())
            loadFile(selectedFileInfo.filePath());
        return;
    }
    else if (loadedPixmap.isNull())
    {
        QMessageBox::critical(this, tr("qView Error"), tr("Error: Null pixmap"));
        if (getIsPixmapLoaded())
            loadFile(selectedFileInfo.filePath());
        return;
    }

    scene()->clear();
    loadedPixmapItem = scene()->addPixmap(loadedPixmap);
    setIsPixmapLoaded(true);
    loadedPixmapItem->setOffset((50000.0 - loadedPixmap.width()/2), (50000.0 - loadedPixmap.height()/2));

    if (getIsFilteringEnabled())
        loadedPixmapItem->setTransformationMode(Qt::SmoothTransformation);
    else
        loadedPixmapItem->setTransformationMode(Qt::FastTransformation);

    resetScale();

    selectedFileInfo = QFileInfo(fileName);
    const QDir fileDir = QDir(selectedFileInfo.path());
    loadedFileFolder = fileDir.entryInfoList(filterList, QDir::Files);
    loadedFileFolderIndex = loadedFileFolder.indexOf(selectedFileInfo);


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

    qobject_cast<QVApplication*>qApp->updateRecentMenus();
}

void QVGraphicsView::setWindowTitle()
{
    if (!isPixmapLoaded)
        return;

    parentMainWindow->refreshProperties();

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
        parentMainWindow->setWindowTitle("qView - "  + selectedFileInfo.fileName() + " - " + QString::number(loadedPixmap.width()) + "x" + QString::number(loadedPixmap.height()) + " - " + locale.formattedDataSize(selectedFileInfo.size()));
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

    if (!getIsScalingEnabled())
        loadedPixmapItem->setPixmap(loadedPixmap);

    isOriginalSize = false;
    calculateBoundingBox();
    fitInViewMarginless(alternateBoundingBox, Qt::KeepAspectRatio);

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
        //4 is added to these numbers to take into account the -2 margin from fitInViewMarginless (kind of a misnomer, eh?)
        qreal marginHeight = (height()-alternateBoundingBox.height()*transform().m22())+4;
        qreal marginWidth = (width()-alternateBoundingBox.width()*transform().m11())+4;
        if (marginWidth < marginHeight)
        {
            scaledPixmap = loadedPixmap.scaledToWidth(width()+4, Qt::SmoothTransformation);
        }
        else
        {
            scaledPixmap = loadedPixmap.scaledToHeight(height()+4, Qt::SmoothTransformation);
        }
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

    if (mode == scaleMode::resetScale)
    {
        calculateBoundingBox();
        fitInViewMarginless(alternateBoundingBox, Qt::KeepAspectRatio);
    }
}

void QVGraphicsView::calculateBoundingBox()
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
}

void QVGraphicsView::originalSize()
{
    loadedPixmapItem->setPixmap(loadedPixmap);
    scale(1,1);
    centerOn(loadedPixmapItem);
    isOriginalSize = true;
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
    if (isPixmapLoaded)
    {
        return loadedPixmap.height();
    }
    else
    {
        return 0;
    }
}

int QVGraphicsView::getImageWidth()
{
    if (isPixmapLoaded)
    {
        return loadedPixmap.width();
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
