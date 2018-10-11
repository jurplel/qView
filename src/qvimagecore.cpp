#include "qvimagecore.h"
#include <QMessageBox>
#include <QDir>
#include <QSettings>

QVImageCore::QVImageCore(QObject *parent) : QObject(parent)
{
    loadedImage = QImage();
    loadedPixmap = QPixmap();
    imageReader.setDecideFormatFromContent(true);

    currentFileDetails.fileInfo = QFileInfo();
    currentFileDetails.isPixmapLoaded = false;
    currentFileDetails.isMovieLoaded = false;
    currentFileDetails.folder = QFileInfoList();
    currentFileDetails.folderIndex = -1;

    connect(&loadedMovie, &QMovie::updated, this, &QVImageCore::animatedFrameChanged);
}

QString QVImageCore::loadFile(const QString &fileName)
{
    imageReader.setFileName(fileName);
    loadedImage = imageReader.read();
    if (loadedImage.isNull())
    {
        return QString::number(imageReader.error()) + ": " + imageReader.errorString();
    }
    loadedPixmap.convertFromImage(loadedImage);

    //animation detection
    if (currentFileDetails.isMovieLoaded)
    {
        loadedMovie.stop();
        currentFileDetails.isMovieLoaded = false;
//        movieCenterNeedsUpdating = false;
    }
    if (imageReader.supportsAnimation() && imageReader.imageCount() != 1)
    {
        loadedMovie.setFileName(fileName);
        loadedMovie.start();
//        movieCenterNeedsUpdating = true;
        currentFileDetails.isMovieLoaded = true;
    }

    //define info variables
    currentFileDetails.fileInfo = QFileInfo(fileName);
    currentFileDetails.isPixmapLoaded = true;
    updateFolderInfo();
    return "";
}

void QVImageCore::updateFolderInfo()
{
    currentFileDetails.folder = QDir(currentFileDetails.fileInfo.path()).entryInfoList(filterList, QDir::Files);
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
        //movieCenterNeedsUpdating = true;
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
