#include "qvimagecore.h"
#include "qvapplication.h"
#include <QMessageBox>
#include <QDir>
#include <QUrl>
#include <QSettings>
#include <QCollator>
#include <QtConcurrent/QtConcurrentRun>
#include <QPixmapCache>
#include <QIcon>
#include <QGuiApplication>
#include <QScreen>

QVImageCore::QVImageCore(QObject *parent) : QObject(parent)
{
    loadedPixmap = QPixmap();
    imageReader.setDecideFormatFromContent(true);
    imageReader.setAutoTransform(true);

    justLoadedFromCache = false;

    isLoopFoldersEnabled = true;
    preloadingMode = 1;
    sortMode = 0;
    sortAscending = true;

    currentFileDetails.fileInfo = QFileInfo();
    currentFileDetails.isPixmapLoaded = false;
    currentFileDetails.isMovieLoaded = false;
    currentFileDetails.folder = QFileInfoList();
    currentFileDetails.folderIndex = -1;
    currentFileDetails.imageSize = QSize();
    currentFileDetails.loadedPixmapSize = QSize();

    lastFileDetails.fileInfo = QFileInfo();
    lastFileDetails.isPixmapLoaded = false;
    lastFileDetails.isMovieLoaded = false;
    lastFileDetails.folder = QFileInfoList();
    lastFileDetails.folderIndex = -1;
    lastFileDetails.imageSize = QSize();
    lastFileDetails.loadedPixmapSize = QSize();

    currentRotation = 0;

    devicePixelRatio = 0;

    QPixmapCache::setCacheLimit(51200);

    connect(&loadedMovie, &QMovie::updated, this, &QVImageCore::animatedFrameChanged);

    connect(&loadFutureWatcher, &QFutureWatcher<QVImageAndFileInfo>::finished, this, [this](){
        postRead(loadFutureWatcher.result());
    });

    fileChangeRateTimer = new QTimer(this);
    fileChangeRateTimer->setSingleShot(true);
    fileChangeRateTimer->setInterval(60);

    largestDimension = 0;
    const auto screenList = QGuiApplication::screens();
    for (auto const &screen : screenList)
    {
        int largerDimension;
        if (screen->size().height() > screen->size().width())
        {
            largerDimension = screen->size().height();
        }
        else
        {
            largerDimension = screen->size().width();
        }

        if (largerDimension > largestDimension)
        {
            largestDimension = largerDimension;
        }
    }
}

void QVImageCore::loadFile(const QString &fileName)
{
    if (loadFutureWatcher.isRunning() || fileChangeRateTimer->isActive())
        return;

    //save old file details in event of a read error
    lastFileDetails = currentFileDetails;

    QString sanitaryString = fileName;

    //sanitize file name if necessary
    QUrl sanitaryUrl = QUrl(fileName);
    if (sanitaryUrl.isLocalFile())
        sanitaryString = sanitaryUrl.toLocalFile();

    //define info variables
    currentFileDetails.fileInfo = QFileInfo(sanitaryString);
    setPaused(true);
    updateFolderInfo();

    imageReader.setFileName(currentFileDetails.fileInfo.absoluteFilePath());
    currentFileDetails.imageSize = imageReader.size();

    auto previouslyRecordedFileSize = qvApp->getPreviouslyRecordedFileSize(currentFileDetails.fileInfo.absoluteFilePath());

    auto *cachedPixmap = new QPixmap();

    //check if cached already before loading the long way
    if (QPixmapCache::find(currentFileDetails.fileInfo.absoluteFilePath(), cachedPixmap) &&
        !cachedPixmap->isNull() &&
        cachedPixmap->size() == currentFileDetails.imageSize &&
        previouslyRecordedFileSize == currentFileDetails.fileInfo.size())
    {
        loadedPixmap = matchCurrentRotation(*cachedPixmap);
        justLoadedFromCache = true;
        postLoad();
    }
    else
    {
        justLoadedFromCache = false;
        loadFutureWatcher.setFuture(QtConcurrent::run(this, &QVImageCore::readFile, currentFileDetails.fileInfo.absoluteFilePath()));
    }
    delete cachedPixmap;

    requestCaching();
}

QVImageCore::QVImageAndFileInfo QVImageCore::readFile(const QString &fileName)
{
    QVImageAndFileInfo combinedInfo;

    QImageReader newImageReader;
    newImageReader.setDecideFormatFromContent(true);
    newImageReader.setAutoTransform(true);

    newImageReader.setFileName(fileName);

    QImage readImage;
    if (newImageReader.format() == "svg" || newImageReader.format() == "svgz")
    {
        QIcon icon;
        icon.addFile(fileName);
        readImage = icon.pixmap(largestDimension).toImage();

    }
    else {
        readImage = newImageReader.read();
    }

    combinedInfo.readFileInfo = QFileInfo(fileName);
    // Only error out when not loading for cache
    if (readImage.isNull() && fileName == currentFileDetails.fileInfo.absoluteFilePath())
    {
        emit readError(QString::number(newImageReader.error()) + ": " + newImageReader.errorString(), fileName);
        currentFileDetails = lastFileDetails;
    }

    combinedInfo.readImage = readImage;
    return combinedInfo;
}

void QVImageCore::postRead(const QVImageAndFileInfo &readImageAndFileInfo)
{
    if (readImageAndFileInfo.readImage.isNull() || justLoadedFromCache)
        return;

    addToCache(readImageAndFileInfo);
    loadedPixmap = QPixmap::fromImage(matchCurrentRotation(readImageAndFileInfo.readImage));
    postLoad();
}

void QVImageCore::postLoad()
{
    loadedPixmap.setDevicePixelRatio(devicePixelRatio);

    fileChangeRateTimer->start();

    currentFileDetails.isPixmapLoaded = true;
    loadedMovie.stop();
    loadedMovie.setFileName("");

    //animation detection
    imageReader.setFileName(currentFileDetails.fileInfo.absoluteFilePath());
    if (imageReader.supportsAnimation() && imageReader.imageCount() != 1)
    {
        loadedMovie.setFileName(currentFileDetails.fileInfo.absoluteFilePath());
        loadedMovie.start();
        currentFileDetails.isMovieLoaded = true;
    }
    else
    {
        currentFileDetails.isMovieLoaded = false;
    }

    currentFileDetails.imageSize = imageReader.size();
    currentFileDetails.loadedPixmapSize = loadedPixmap.size();

    emit fileRead();
    emit fileInfoUpdated();
}

void QVImageCore::updateFolderInfo()
{
    QCollator collator;
    collator.setNumericMode(true);
    QDir::SortFlags sortFlags = QDir::NoSort;

    switch (sortMode) {
    case 1: {
        sortFlags = QDir::Time;
        break;
    }
    case 2: {
        sortFlags = QDir::Size;
        break;
    }
    case 3: {
        sortFlags = QDir::Type;
        break;
    }
    }

    if (!sortAscending)
        sortFlags.setFlag(QDir::Reversed, true);

    currentFileDetails.folder = QDir(currentFileDetails.fileInfo.absolutePath()).entryInfoList(qvApp->getFilterList(), QDir::Files, sortFlags);

    // Natural sorting
    if (sortMode == 0) {
        std::sort(currentFileDetails.folder.begin(), currentFileDetails.folder.end(), [&collator, this](const QFileInfo &file1, const QFileInfo &file2) {
            if (sortAscending)
                return collator.compare(file1.fileName(), file2.fileName()) < 0;
            else
                return collator.compare(file1.fileName(), file2.fileName()) > 0;
        });
    }

    currentFileDetails.folderIndex = currentFileDetails.folder.indexOf(currentFileDetails.fileInfo);
}

void QVImageCore::requestCaching()
{
    if (preloadingMode == 0)
    {
        QPixmapCache::clear();
        return;
    }

    int preloadingDistance = 1;

    if (preloadingMode > 1)
        preloadingDistance = 4;

    QStringList filesToPreload;
    for (int i = currentFileDetails.folderIndex-preloadingDistance; i <= currentFileDetails.folderIndex+preloadingDistance; i++)
    {
        int index = i;
        //keep within index range
        if (isLoopFoldersEnabled)
        {
            if (index > currentFileDetails.folder.length()-1)
                index = index-(currentFileDetails.folder.length());
            else if (index < 0)
                index = index+(currentFileDetails.folder.length());
        }

        //if still out of range after looping, just cancel the cache for this index
        if (index > currentFileDetails.folder.length()-1 || index < 0 || currentFileDetails.folder.isEmpty())
            return;

        QString filePath = currentFileDetails.folder[index].absoluteFilePath();
        filesToPreload.append(filePath);

        requestCachingFile(filePath);
    }
    lastFilesPreloaded = filesToPreload;

}

void QVImageCore::requestCachingFile(const QString &filePath)
{
    //check if image is already loaded or requested
    if (QPixmapCache::find(filePath, nullptr) || lastFilesPreloaded.contains(filePath))
        return;

    //check if too big for caching
    imageReader.setFileName(filePath);
    QTransform transform;
    transform.rotate(currentRotation);
    if (((imageReader.size().width()*imageReader.size().height()*32)/8)/1000 > QPixmapCache::cacheLimit()/2)
        return;

    auto *cacheFutureWatcher = new QFutureWatcher<QVImageAndFileInfo>();
    connect(cacheFutureWatcher, &QFutureWatcher<QVImageAndFileInfo>::finished, this, [cacheFutureWatcher, this](){
        addToCache(cacheFutureWatcher->result());
        cacheFutureWatcher->deleteLater();
    });
    cacheFutureWatcher->setFuture(QtConcurrent::run(this, &QVImageCore::readFile, filePath));
}

void QVImageCore::addToCache(const QVImageAndFileInfo &readImageAndFileInfo)
{
    if (readImageAndFileInfo.readImage.isNull())
        return;

    QPixmapCache::insert(readImageAndFileInfo.readFileInfo.absoluteFilePath(), QPixmap::fromImage(readImageAndFileInfo.readImage));

    auto *size = new qint64(readImageAndFileInfo.readFileInfo.size());
    qvApp->setPreviouslyRecordedFileSize(readImageAndFileInfo.readFileInfo.absoluteFilePath(), size);
}

void QVImageCore::jumpToNextFrame()
{
    if (currentFileDetails.isMovieLoaded)
        loadedMovie.jumpToNextFrame();
}

void QVImageCore::setPaused(bool desiredState)
{
    if (currentFileDetails.isMovieLoaded)
        loadedMovie.setPaused(desiredState);
}

void QVImageCore::setSpeed(int desiredSpeed)
{
    if (currentFileDetails.isMovieLoaded)
        loadedMovie.setSpeed(desiredSpeed);
}

void QVImageCore::rotateImage(int rotation)
{
        currentRotation += rotation;

        // normalize between 360 and 0
        currentRotation = (currentRotation % 360 + 360) % 360;
        QTransform transform;

        QImage transformedImage;
        if (currentFileDetails.isMovieLoaded)
        {
            transform.rotate(currentRotation);
            transformedImage = loadedMovie.currentImage().transformed(transform);
        }
        else
        {
            transform.rotate(rotation);
            transformedImage = loadedPixmap.toImage().transformed(transform);
        }

        loadedPixmap.convertFromImage(transformedImage);

        currentFileDetails.loadedPixmapSize = QSize(loadedPixmap.width(), loadedPixmap.height());
        emit updateLoadedPixmapItem();
}

QImage QVImageCore::matchCurrentRotation(const QImage &imageToRotate)
{
    if (!currentRotation)
        return imageToRotate;

    QTransform transform;
    transform.rotate(currentRotation);
    return imageToRotate.transformed(transform);
}

QPixmap QVImageCore::matchCurrentRotation(const QPixmap &pixmapToRotate)
{
    if (!currentRotation)
        return pixmapToRotate;

    return QPixmap::fromImage(matchCurrentRotation(pixmapToRotate.toImage()));
}

QPixmap QVImageCore::scaleExpensively(const int desiredWidth, const int desiredHeight, const scaleMode mode)
{
    return scaleExpensively(QSize(desiredWidth, desiredHeight), mode);
}

QPixmap QVImageCore::scaleExpensively(const QSize desiredSize, const scaleMode mode)
{
    if (!currentFileDetails.isPixmapLoaded)
        return QPixmap();

    QSize size = QSize(loadedPixmap.width(), loadedPixmap.height());
    size.scale(desiredSize, Qt::KeepAspectRatio);

    QPixmap relevantPixmap;
    if (!currentFileDetails.isMovieLoaded)
    {
        relevantPixmap = loadedPixmap;
    }
    else
    {
        relevantPixmap = loadedMovie.currentPixmap();
        relevantPixmap = matchCurrentRotation(relevantPixmap);
    }

    switch (mode) {
    case scaleMode::normal:
    {
        relevantPixmap = relevantPixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        break;
    }
    case scaleMode::width:
    {
        relevantPixmap = relevantPixmap.scaledToWidth(desiredSize.width(), Qt::SmoothTransformation);
        break;
    }
    case scaleMode::height:
    {
        relevantPixmap = relevantPixmap.scaledToHeight(desiredSize.height(), Qt::SmoothTransformation);
        break;
    }
    }

    return relevantPixmap;
}


void QVImageCore::loadSettings()
{
    QSettings settings;
    settings.beginGroup("options");

    //loop folders
    isLoopFoldersEnabled = settings.value("loopfoldersenabled", true).toBool();

    //preloading mode
    preloadingMode = settings.value("preloadingmode", 1).toInt();
    switch (preloadingMode) {
    case 1:
    {
        QPixmapCache::setCacheLimit(51200);
        break;
    }
    case 2:
    {
        QPixmapCache::setCacheLimit(204800);
        break;
    }
    }

    //sort mode
    sortMode = settings.value("sortmode", 0).toInt();

    //sort ascending
    sortAscending = settings.value("sortascending", true).toBool();

    //update folder info to re-sort
    updateFolderInfo();
}

void QVImageCore::setDevicePixelRatio(qreal scaleFactor)
{
    devicePixelRatio = scaleFactor;
}
