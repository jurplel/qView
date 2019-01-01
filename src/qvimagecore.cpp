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
    preloadingMode = 1;

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

    pixmapCache.setCacheLimit(512000);

    connect(&loadedMovie, &QMovie::updated, this, &QVImageCore::animatedFrameChanged);

    connect(&loadFutureWatcher, &QFutureWatcher<QVImageAndFileInfo>::finished, this, &QVImageCore::processFile);

    fileChangeRateTimer = new QTimer(this);
    fileChangeRateTimer->setSingleShot(true);
    fileChangeRateTimer->setInterval(60);
}

void QVImageCore::loadFile(const QString &fileName)
{
    if (loadFutureWatcher.isRunning() || fileChangeRateTimer->isActive())
        return;

    lastFileDetails = currentFileDetails;

    requestCaching();

    //define info variables
    currentFileDetails.isMovieLoaded = false;
    currentFileDetails.fileInfo = QFileInfo(fileName);
    updateFolderInfo();

    imageReader.setFileName(fileName);
    currentFileDetails.imageSize = imageReader.size();

    if (pixmapCache.find(currentFileDetails.fileInfo.filePath(), loadedPixmap))
    {
        vetoFutureWatcher = true;
        postLoad();
    }
    else
    {
        vetoFutureWatcher = false;
        loadFutureWatcher.setFuture(QtConcurrent::run(this, &QVImageCore::readFile, currentFileDetails.fileInfo.filePath()));
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

void QVImageCore::processFile()
{
    if (vetoFutureWatcher)
        return;

    QVImageAndFileInfo loadedImageAndFileInfo = loadFutureWatcher.result();

    if (loadedImageAndFileInfo.readImage.isNull())
        return;

    loadedPixmap.convertFromImage(loadedImageAndFileInfo.readImage);
    postLoad();
}

void QVImageCore::postLoad()
{
    fileChangeRateTimer->start();

    currentFileDetails.isPixmapLoaded = true;
    loadedMovie.stop();
    loadedMovie.setFileName("");

    fileInfoUpdated();

    //animation detection
    imageReader.setFileName(currentFileDetails.fileInfo.filePath());
    if (imageReader.supportsAnimation() && imageReader.imageCount() != 1)
    {
        loadedMovie.setFileName(currentFileDetails.fileInfo.filePath());
        loadedMovie.setScaledSize(loadedPixmap.size());
        loadedMovie.start();
        currentFileDetails.isMovieLoaded = true;
    }

    currentFileDetails.imageSize = QSize(loadedPixmap.width(), loadedPixmap.height());

    emit fileRead(currentFileDetails.fileInfo.path());
    emit fileInfoUpdated();
}

void QVImageCore::requestCaching()
{
    if (preloadingMode > 0)
    {
        int preloadingDistance = 1;

        if (preloadingMode > 1)
            preloadingDistance = 4;

        for (int i = currentFileDetails.folderIndex-preloadingDistance; i <= currentFileDetails.folderIndex+preloadingDistance; i++)
            addIndexToCache(i);
    }
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

void QVImageCore::addIndexToCache(int index)
{
    if (currentFileDetails.folder.isEmpty())
        return;

    //keep within index range
    if (index > currentFileDetails.folder.length()-1)
    {
        if (!isLoopFoldersEnabled)
            return;
        index = index-(currentFileDetails.folder.length());
    }
    else if (index < 0)
    {
        if (!isLoopFoldersEnabled)
            return;
        index = index+(currentFileDetails.folder.length());
    }

    QString filePath = currentFileDetails.folder[index].filePath();

    if (pixmapCache.find(filePath, nullptr))
        return;

    QFutureWatcher<QVImageAndFileInfo> *cacheFutureWatcher = new QFutureWatcher<QVImageAndFileInfo>();
    connect(cacheFutureWatcher, &QFutureWatcher<QVImageAndFileInfo>::finished, [cacheFutureWatcher, this](){
        addToCache(cacheFutureWatcher->result());
        cacheFutureWatcher->deleteLater();
    });
    cacheFutureWatcher->setFuture(QtConcurrent::run(this, &QVImageCore::readFile, filePath));
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

    //preloading mode
    preloadingMode = settings.value("preloadingmode", 1).toInt();
}
