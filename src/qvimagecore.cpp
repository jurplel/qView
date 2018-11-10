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
    loadedImage = QImage();
    loadedPixmap = QPixmap();
    imageReader.setDecideFormatFromContent(true);
    imageReader.setAutoTransform(true);

    currentFileDetails.fileInfo = QFileInfo();
    currentFileDetails.isPixmapLoaded = false;
    currentFileDetails.isMovieLoaded = false;
    currentFileDetails.folder = QFileInfoList();
    currentFileDetails.folderIndex = -1;

    connect(&loadedMovie, &QMovie::updated, this, &QVImageCore::animatedFrameChanged);

    connect(&futureWatcher, &QFutureWatcher<imageAndFileInfo>::resultReadyAt, this, &QVImageCore::processFile);
}

void QVImageCore::loadFile(const QString &fileName)
{
    QThreadPool *globalThreadPool = QThreadPool::globalInstance();
    if (globalThreadPool->activeThreadCount() < globalThreadPool->maxThreadCount())
        futureWatcher.setFuture(QtConcurrent::run(this, &QVImageCore::readFile, fileName));

    //define info variables
    currentFileDetails.fileInfo = QFileInfo(fileName);
    currentFileDetails.isPixmapLoaded = true;
    updateFolderInfo();

    imageReader.setFileName(fileName);
    currentFileDetails.imageSize = imageReader.size();

    emit fileInfoUpdated();
}

QVImageCore::imageAndFileInfo QVImageCore::readFile(const QString &fileName)
{
    imageAndFileInfo combinedInfo;

    QImageReader newImageReader;
    newImageReader.setDecideFormatFromContent(true);
    newImageReader.setAutoTransform(true);

    newImageReader.setFileName(fileName);
    const QImage readImage = newImageReader.read();

    combinedInfo.readFileInfo = QFileInfo(fileName);
    if (readImage.isNull())
    {
        emit readError(QString::number(newImageReader.error()) + ": " + newImageReader.errorString(), fileName);
        return combinedInfo;
    }

    combinedInfo.readImage = readImage;
    return combinedInfo;
}

void QVImageCore::processFile(int index)
{
    if (futureWatcher.isRunning())
        return;

    imageAndFileInfo loadedImageAndFileInfo = futureWatcher.resultAt(index);
    if (loadedImageAndFileInfo.readImage.isNull())
        return;

    loadedImage = loadedImageAndFileInfo.readImage;
    loadedPixmap.convertFromImage(loadedImage);

    //animation detection
    imageReader.setFileName(loadedImageAndFileInfo.readFileInfo.filePath());
    if (currentFileDetails.isMovieLoaded)
    {
        loadedMovie.stop();
        currentFileDetails.isMovieLoaded = false;
    }
    if (imageReader.supportsAnimation() && imageReader.imageCount() != 1)
    {
        loadedMovie.setFileName(loadedImageAndFileInfo.readFileInfo.filePath());
        loadedMovie.setScaledSize(loadedPixmap.size());
        loadedMovie.start();
        currentFileDetails.isMovieLoaded = true;
    }

    emit fileRead(currentFileDetails.fileInfo.path());
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


