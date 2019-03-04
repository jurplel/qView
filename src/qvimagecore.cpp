#include "qvimagecore.h"
#include <QMessageBox>
#include <QDir>
#include <QUrl>
#include <QSettings>
#include <QCollator>
#include <QtConcurrent/QtConcurrentRun>
#include <QPixmapCache>

#include <QDebug>

QVImageCore::QVImageCore(QObject *parent) : QObject(parent)
{
    loadedPixmap = QPixmap();
    imageReader.setDecideFormatFromContent(true);
    imageReader.setAutoTransform(true);

    justLoadedFromCache = false;

    isLoopFoldersEnabled = true;
    preloadingMode = 1;

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

    QPixmapCache::setCacheLimit(51200);

    connect(&loadedMovie, &QMovie::updated, this, &QVImageCore::animatedFrameChanged);

    connect(&loadFutureWatcher, &QFutureWatcher<QVImageAndFileInfo>::finished, this, [this](){
        postRead(loadFutureWatcher.result().readImage);
    });

    fileChangeRateTimer = new QTimer(this);
    fileChangeRateTimer->setSingleShot(true);
    fileChangeRateTimer->setInterval(60);
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
    currentFileDetails.isMovieLoaded = false;
    currentFileDetails.fileInfo = QFileInfo(sanitaryString);
    updateFolderInfo();

    imageReader.setFileName(currentFileDetails.fileInfo.absoluteFilePath());
    currentFileDetails.imageSize = imageReader.size();

    //check if cached already before loading the long way
    QPixmap cachedPixmap;
    if (QPixmapCache::find(currentFileDetails.fileInfo.absoluteFilePath(), cachedPixmap) &&
        !cachedPixmap.isNull() &&
        cachedPixmap.size() == currentFileDetails.imageSize &&
        *previouslyRecordedFileSizes.object(currentFileDetails.fileInfo.absoluteFilePath()) == currentFileDetails.fileInfo.size())
    {
        loadedPixmap = matchCurrentRotation(cachedPixmap);
        justLoadedFromCache = true;
        postLoad();
    }
    else
    {
        justLoadedFromCache = false;
        loadFutureWatcher.setFuture(QtConcurrent::run(this, &QVImageCore::readFile, currentFileDetails.fileInfo.absoluteFilePath()));
    }

    requestCaching();
}

QVImageCore::QVImageAndFileInfo QVImageCore::readFile(const QString &fileName)
{
    QVImageAndFileInfo combinedInfo;

    QImageReader newImageReader;
    newImageReader.setDecideFormatFromContent(true);
    newImageReader.setAutoTransform(true);

    QPixmapCache::find(fileName);

    newImageReader.setFileName(fileName);
    QImage readImage = newImageReader.read();

    combinedInfo.readFileInfo = QFileInfo(fileName);
    if (readImage.isNull())
    {
        emit readError(QString::number(newImageReader.error()) + ": " + newImageReader.errorString(), fileName);
        currentFileDetails = lastFileDetails;
        return combinedInfo;
    }

    combinedInfo.readImage = readImage;
    return combinedInfo;
}

void QVImageCore::postRead(const QImage &loadedImage)
{
    if (loadedImage.isNull() || justLoadedFromCache)
        return;

    loadedPixmap = QPixmap::fromImage(matchCurrentRotation(loadedImage));
    postLoad();
}

void QVImageCore::postLoad()
{
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

    currentFileDetails.imageSize = imageReader.size();
    currentFileDetails.loadedPixmapSize = loadedPixmap.size();

    emit fileRead();
    emit fileInfoUpdated();
}

void QVImageCore::updateFolderInfo()
{
    QCollator collator;
    collator.setNumericMode(true);
    currentFileDetails.folder = QDir(currentFileDetails.fileInfo.path()).entryInfoList(filterList, QDir::Files, QDir::NoSort);
    std::sort(
        currentFileDetails.folder.begin(),
        currentFileDetails.folder.end(),
        [&collator](const QFileInfo &file1, const QFileInfo &file2)
        {
            return collator.compare(file1.fileName(), file2.fileName()) < 0;
        });
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

void QVImageCore::addToCache(QVImageAndFileInfo loadedImageAndFileInfo)
{
    if (loadedImageAndFileInfo.readImage.isNull())
        return;

    QPixmapCache::insert(loadedImageAndFileInfo.readFileInfo.absoluteFilePath(), QPixmap::fromImage(loadedImageAndFileInfo.readImage));

    auto *size = new qint64(loadedImageAndFileInfo.readFileInfo.size());
    previouslyRecordedFileSizes.insert(loadedImageAndFileInfo.readFileInfo.absoluteFilePath(), size);
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

const QImage QVImageCore::matchCurrentRotation(const QImage &imageToRotate)
{
    if (!currentRotation)
        return imageToRotate;

    QTransform transform;
    transform.rotate(currentRotation);
    return imageToRotate.transformed(transform);
}

const QPixmap QVImageCore::matchCurrentRotation(const QPixmap &pixmapToRotate)
{
    if (!currentRotation)
        return pixmapToRotate;

    return QPixmap::fromImage(matchCurrentRotation(pixmapToRotate.toImage()));
}

const QPixmap QVImageCore::scaleExpensively(const int desiredWidth, const int desiredHeight, const scaleMode mode)
{
    return scaleExpensively(QSize(desiredWidth, desiredHeight), mode);
}

const QPixmap QVImageCore::scaleExpensively(const QSize desiredSize, const scaleMode mode)
{
    if (!currentFileDetails.isPixmapLoaded)
        return QPixmap();

    QSize size = QSize(loadedPixmap.width(), loadedPixmap.height());
    size.scale(desiredSize, Qt::KeepAspectRatio);

    if (!currentFileDetails.isMovieLoaded)
    {
        switch (mode) {
        case scaleMode::normal:
        {
            return loadedPixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        case scaleMode::width:
        {
            return loadedPixmap.scaledToWidth(desiredSize.width(), Qt::SmoothTransformation);
        }
        case scaleMode::height:
        {
            return loadedPixmap.scaledToHeight(desiredSize.height(), Qt::SmoothTransformation);
        }
        }
    }
    QTransform transform;
    transform.rotate(currentRotation);
    QImage image = loadedMovie.currentImage().transformed(transform);
    return QPixmap::fromImage(image).scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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
}
