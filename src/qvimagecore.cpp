#include "qvimagecore.h"
#include <QMessageBox>
#include <QDir>
#include <QSettings>
#include <QCollator>
#include <QtConcurrent/QtConcurrent>
#include <QThreadPool>

#include <QDebug>

QVImageCore::QVImageCore(QObject *parent) : QObject(parent)
{
    loadedPixmap = QPixmap();
    imageReader.setDecideFormatFromContent(true);
    imageReader.setAutoTransform(true);

    vetoFutureWatcher = false;

    isLoopFoldersEnabled = true;

    currentFileDetails.fileInfo = QFileInfo();
    currentFileDetails.isPixmapLoaded = false;
    currentFileDetails.isMovieLoaded = false;
    currentFileDetails.folder = QFileInfoList();
    currentFileDetails.folderIndex = -1;
    currentFileDetails.imageSize = QSize();

    lastFileDetails.fileInfo = QFileInfo();
    lastFileDetails.isPixmapLoaded = false;
    lastFileDetails.isMovieLoaded = false;
    lastFileDetails.folder = QFileInfoList();
    lastFileDetails.folderIndex = -1;
    lastFileDetails.imageSize = QSize();

    pixmapCache.setCacheLimit(102400);

    connect(&loadedMovie, &QMovie::updated, this, &QVImageCore::animatedFrameChanged);

    connect(&loadFutureWatcher, &QFutureWatcher<QVImageAndFileInfo>::resultReadyAt, this, &QVImageCore::processFile);
    connect(&cacheFutureWatcher, &QFutureWatcher<QVImageAndFileInfo>::resultReadyAt, [this](const int index){addToCache(cacheFutureWatcher.resultAt(index));});
    connect(&cacheFutureWatcher2, &QFutureWatcher<QVImageAndFileInfo>::resultReadyAt, [this](const int index){addToCache(cacheFutureWatcher2.resultAt(index)); });

    cacheTimer = new QTimer(this);
    cacheTimer->setSingleShot(true);
    cacheTimer->setInterval(100);
    connect(cacheTimer, &QTimer::timeout, this, &QVImageCore::requestCaching);
}

void QVImageCore::loadFile(const QString &fileName)
{
    lastFileDetails = currentFileDetails;

    //define info variables
    currentFileDetails.fileInfo = QFileInfo(fileName);
    currentFileDetails.isPixmapLoaded = true;
    currentFileDetails.isMovieLoaded = false;
    updateFolderInfo();

    imageReader.setFileName(fileName);
    currentFileDetails.imageSize = imageReader.size();

    emit fileInfoUpdated();

    if (!pixmapCache.find(currentFileDetails.fileInfo.filePath(), loadedPixmap))
    {
        qDebug() << 0;
        vetoFutureWatcher = false;
        QThreadPool *globalThreadPool = QThreadPool::globalInstance();
        if (globalThreadPool->activeThreadCount() < globalThreadPool->maxThreadCount())
            loadFutureWatcher.setFuture(QtConcurrent::run(this, &QVImageCore::readFile, currentFileDetails.fileInfo.filePath()));
    }
    else
    {
        qDebug() << 1;
        vetoFutureWatcher = true;
        postLoad();
        pixmapCache.remove(currentFileDetails.fileInfo.filePath());
    }

}

QVImageCore::QVImageAndFileInfo QVImageCore::readFile(const QString &fileName)
{
    QVImageAndFileInfo combinedInfo;

    QImageReader newImageReader;
    newImageReader.setDecideFormatFromContent(true);
    newImageReader.setAutoTransform(true);

    newImageReader.setFileName(fileName);
    const QImage readImage = newImageReader.read();

    combinedInfo.readFileInfo = QFileInfo(fileName);
    if (readImage.isNull())
    {
        emit readError(QString::number(newImageReader.error()) + ": " + newImageReader.errorString(), fileName);
        currentFileDetails = lastFileDetails;
        emit fileInfoUpdated();
        return combinedInfo;
    }

    combinedInfo.readImage = readImage;
    return combinedInfo;
}

void QVImageCore::processFile(int index)
{
    if (loadFutureWatcher.isRunning() || vetoFutureWatcher)
        return;

    QVImageAndFileInfo loadedImageAndFileInfo = loadFutureWatcher.resultAt(index);
    if (loadedImageAndFileInfo.readImage.isNull())
        return;

    loadedPixmap.convertFromImage(loadedImageAndFileInfo.readImage);
    postLoad();
}

void QVImageCore::postLoad()
{
    //animation detection
    imageReader.setFileName(currentFileDetails.fileInfo.filePath());
    loadedMovie.stop();
    if (imageReader.supportsAnimation() && imageReader.imageCount() != 1)
    {
        loadedMovie.setFileName(currentFileDetails.fileInfo.filePath());
        loadedMovie.setScaledSize(loadedPixmap.size());
        loadedMovie.start();
        currentFileDetails.isMovieLoaded = true;
    }

    currentFileDetails.imageSize = QSize(loadedPixmap.width(), loadedPixmap.height());

    emit fileRead(currentFileDetails.fileInfo.path());

    cacheTimer->start();
}

void QVImageCore::requestCaching()
{
    pixmapCache.clear();
    //add previous and next file to cache
    if (currentFileDetails.folderIndex-1 > 0)
        addIndexToCache(currentFileDetails.folderIndex-1);
    else if (isLoopFoldersEnabled)
        addIndexToCache(currentFileDetails.folder.length()-1);

    if (currentFileDetails.folderIndex+1 < currentFileDetails.folder.length()-1)
        addIndexToCache(currentFileDetails.folderIndex+1);
    else if (isLoopFoldersEnabled)
        addIndexToCache(0);
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

void QVImageCore::addIndexToCache(const int &index)
{
    if (currentFileDetails.folder.isEmpty())
        return;

    QString filePath = currentFileDetails.folder[index].filePath();

    if (pixmapCache.find(filePath, nullptr))
        return;

    if (!cacheFutureWatcher.isRunning())
        cacheFutureWatcher.setFuture(QtConcurrent::run(this, &QVImageCore::readFile, filePath));
    else
        cacheFutureWatcher2.setFuture(QtConcurrent::run(this, &QVImageCore::readFile, filePath));
}

void QVImageCore::addToCache(const QVImageAndFileInfo &loadedImageAndFileInfo)
{
    if (loadedImageAndFileInfo.readImage.isNull())
        return;

    pixmapCache.insert(loadedImageAndFileInfo.readFileInfo.filePath(), QPixmap::fromImage(loadedImageAndFileInfo.readImage));
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

    if (currentFileDetails.isMovieLoaded)
    {
        loadedMovie.setScaledSize(size);
        return loadedMovie.currentPixmap().scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    else
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
    return QPixmap();
}


void QVImageCore::loadSettings()
{
    QSettings settings;
    settings.beginGroup("options");

    //loop folders
    isLoopFoldersEnabled = settings.value("loopfoldersenabled", true).toBool();
}
