#include "qvimagecore.h"
#include "qvapplication.h"
#include <random>
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
// Set allocation limit to 8 GiB on Qt6
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QImageReader::setAllocationLimit(8192);
#endif

    isLoopFoldersEnabled = true;
    preloadingMode = 1;
    sortMode = 0;
    sortDescending = false;
    allowMimeContentDetection = false;

    randomSortSeed = 0;

    currentRotation = 0;

    QPixmapCache::setCacheLimit(51200);

    connect(&loadedMovie, &QMovie::updated, this, &QVImageCore::animatedFrameChanged);

    connect(&loadFutureWatcher, &QFutureWatcher<ReadData>::finished, this, [this](){
        loadPixmap(loadFutureWatcher.result(), false);
    });

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

    waitingOnLoad = false;

    // Connect to settings signal
    connect(&qvApp->getSettingsManager(), &SettingsManager::settingsUpdated, this, &QVImageCore::settingsUpdated);
    settingsUpdated();
}

void QVImageCore::loadFile(const QString &fileName)
{
    if (waitingOnLoad)
    {
        return;
    }

    QString sanitaryFileName = fileName;

    //sanitize file name if necessary
    QUrl sanitaryUrl = QUrl(fileName);
    if (sanitaryUrl.isLocalFile())
        sanitaryFileName = sanitaryUrl.toLocalFile();

    QFileInfo fileInfo(sanitaryFileName);
    sanitaryFileName = fileInfo.absoluteFilePath();

    if (fileInfo.isDir())
    {
        updateFolderInfo(sanitaryFileName);
        if (currentFileDetails.folderFileInfoList.isEmpty())
            closeImage();
        else
            loadFile(currentFileDetails.folderFileInfoList.at(0).absoluteFilePath);
        return;
    }

    // Pause playing movie because it feels better that way
    setPaused(true);

    currentFileDetails.isLoadRequested = true;
    waitingOnLoad = true;


    //check if cached already before loading the long way
    auto previouslyRecordedFileSize = qvApp->getPreviouslyRecordedFileSize(sanitaryFileName);
    auto *cachedPixmap = new QPixmap();
    if (QPixmapCache::find(sanitaryFileName, cachedPixmap) &&
        !cachedPixmap->isNull() &&
        previouslyRecordedFileSize == fileInfo.size())
    {
        QSize previouslyRecordedImageSize = qvApp->getPreviouslyRecordedImageSize(sanitaryFileName);
        ReadData readData = {
            *cachedPixmap,
            fileInfo,
            previouslyRecordedImageSize
        };
        loadPixmap(readData, true);
    }
    else
    {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        loadFutureWatcher.setFuture(QtConcurrent::run(this, &QVImageCore::readFile, sanitaryFileName, false));
#else
        loadFutureWatcher.setFuture(QtConcurrent::run(&QVImageCore::readFile, this, sanitaryFileName, false));
#endif
    }
    delete cachedPixmap;
}

QVImageCore::ReadData QVImageCore::readFile(const QString &fileName, bool forCache)
{
    QImageReader imageReader;
    imageReader.setDecideFormatFromContent(true);
    imageReader.setAutoTransform(true);

    imageReader.setFileName(fileName);

    QPixmap readPixmap;
    if (imageReader.format() == "svg" || imageReader.format() == "svgz")
    {
        // Render vectors into a high resolution
        QIcon icon;
        icon.addFile(fileName);
        readPixmap = icon.pixmap(largestDimension);
        // If this fails, try reading the normal way so that a proper error message is given
        if (readPixmap.isNull())
            readPixmap = QPixmap::fromImageReader(&imageReader);
    }
    else
    {
        readPixmap = QPixmap::fromImageReader(&imageReader);
    }


    ReadData readData = {
        readPixmap,
        QFileInfo(fileName),
        imageReader.size(),
    };
    // Only error out when not loading for cache
    if (readPixmap.isNull() && !forCache)
    {
        emit readError(imageReader.error(), imageReader.errorString(), readData.fileInfo.fileName());
    }

    return readData;
}

void QVImageCore::loadPixmap(const ReadData &readData, bool fromCache)
{
    // Do this first so we can keep folder info even when loading errored files
    currentFileDetails.fileInfo = readData.fileInfo;
    currentFileDetails.updateLoadedIndexInFolder();
    if (currentFileDetails.loadedIndexInFolder == -1)
        updateFolderInfo();

    // Reset mechanism to avoid stalling while loading
    waitingOnLoad = false;

    if (readData.pixmap.isNull())
        return;

    loadedPixmap = matchCurrentRotation(readData.pixmap);

    // Set file details
    currentFileDetails.isPixmapLoaded = true;
    currentFileDetails.baseImageSize = readData.size;
    currentFileDetails.loadedPixmapSize = loadedPixmap.size();
    if (currentFileDetails.baseImageSize == QSize(-1, -1))
    {
        qInfo() << "QImageReader::size gave an invalid size for " + currentFileDetails.fileInfo.fileName() + ", using size from loaded pixmap";
        currentFileDetails.baseImageSize = currentFileDetails.loadedPixmapSize;
    }

    // If this image isnt originally from the cache, add it to the cache
    if (!fromCache)
        addToCache(readData);

    // Animation detection
    loadedMovie.setFormat("");
    loadedMovie.stop();
    loadedMovie.setFileName(currentFileDetails.fileInfo.absoluteFilePath());

    // APNG workaround
    if (loadedMovie.format() == "png")
    {
        loadedMovie.setFormat("apng");
        loadedMovie.setFileName(currentFileDetails.fileInfo.absoluteFilePath());
    }

    currentFileDetails.isMovieLoaded = loadedMovie.isValid() && loadedMovie.frameCount() != 1;

    if (currentFileDetails.isMovieLoaded)
        loadedMovie.start();
    else if (auto device = loadedMovie.device())
        device->close();

    currentFileDetails.timeSinceLoaded.start();

    emit fileChanged();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QtConcurrent::run(this, &QVImageCore::requestCaching);
#else
    QtConcurrent::run(&QVImageCore::requestCaching, this);
#endif
}

void QVImageCore::closeImage()
{
    loadedPixmap = QPixmap();
    loadedMovie.stop();
    loadedMovie.setFileName("");
    currentFileDetails = {
        QFileInfo(),
        currentFileDetails.folderFileInfoList,
        currentFileDetails.loadedIndexInFolder,
        false,
        false,
        false,
        QSize(),
        QSize(),
        QElapsedTimer()
    };

    emit fileChanged();
}

// All file logic, sorting, etc should be moved to a different class or file
QList<QVImageCore::CompatibleFile> QVImageCore::getCompatibleFiles(const QString &dirPath) const
{
    QList<CompatibleFile> fileList;

    QMimeDatabase mimeDb;
    const auto &extensions = qvApp->getFileExtensionList();
    const auto &mimeTypes = qvApp->getMimeTypeNameList();

    QMimeDatabase::MatchMode mimeMatchMode = allowMimeContentDetection ? QMimeDatabase::MatchDefault : QMimeDatabase::MatchExtension;

    const QFileInfoList currentFolder = QDir(dirPath).entryInfoList(QDir::Files | QDir::Hidden, QDir::Unsorted);
    for (const QFileInfo &fileInfo : currentFolder)
    {
        bool matched = false;
        const QString absoluteFilePath = fileInfo.absoluteFilePath();
        const QString fileName = fileInfo.fileName();
        for (const QString &extension : extensions)
        {
            if (fileName.endsWith(extension, Qt::CaseInsensitive))
            {
                matched = true;
                break;
            }
        }
        QString mimeType;
        if (!matched || sortMode == 3)
        {
            mimeType = mimeDb.mimeTypeForFile(absoluteFilePath, mimeMatchMode).name();
            matched |= mimeTypes.contains(mimeType);
        }
        if (matched)
        {
            fileList.append({
                absoluteFilePath,
                fileName,
                sortMode == 1 ? fileInfo.lastModified().toMSecsSinceEpoch() : 0,
                sortMode == 2 ? fileInfo.size() : 0,
                sortMode == 3 ? mimeType : QString()
            });
        }
    }

    return fileList;
}

void QVImageCore::updateFolderInfo(QString dirPath)
{
    if (dirPath.isEmpty())
    {
        dirPath = currentFileDetails.fileInfo.path();

        // No directory specified and a file is not already loaded from which we can infer one
        if (dirPath.isEmpty())
            return;
    }

    currentFileDetails.folderFileInfoList = getCompatibleFiles(dirPath);

    QPair<QString, qsizetype> dirInfo = {currentFileDetails.fileInfo.absoluteDir().path(),
                                         currentFileDetails.folderFileInfoList.count()};
    // If the current folder changed since the last image, assign a new seed for random sorting
    if (lastDirInfo != dirInfo)
    {
        randomSortSeed = std::chrono::system_clock::now().time_since_epoch().count();
    }
    lastDirInfo = dirInfo;

    // Sorting

    if (sortMode == 0) // Natural sorting
    {
        QCollator collator;
        collator.setNumericMode(true);
        std::sort(currentFileDetails.folderFileInfoList.begin(),
                  currentFileDetails.folderFileInfoList.end(),
                  [&collator, this](const CompatibleFile &file1, const CompatibleFile &file2)
        {
            if (sortDescending)
                return collator.compare(file1.fileName, file2.fileName) > 0;
            else
                return collator.compare(file1.fileName, file2.fileName) < 0;
        });
    }
    else if (sortMode == 1) // last modified
    {
        std::sort(currentFileDetails.folderFileInfoList.begin(),
                  currentFileDetails.folderFileInfoList.end(),
                  [this](const CompatibleFile &file1, const CompatibleFile &file2)
        {
            if (sortDescending)
                return file1.lastModified < file2.lastModified;
            else
                return file1.lastModified > file2.lastModified;
        });
    }
    else if (sortMode == 2) // size
    {
        std::sort(currentFileDetails.folderFileInfoList.begin(),
                  currentFileDetails.folderFileInfoList.end(),
                  [this](const CompatibleFile &file1, const CompatibleFile &file2)
        {
            if (sortDescending)
                return file1.size < file2.size;
            else
                return file1.size > file2.size;
        });
    }
    else if (sortMode == 3) // type
    {
        QCollator collator;
        std::sort(currentFileDetails.folderFileInfoList.begin(),
                  currentFileDetails.folderFileInfoList.end(),
                  [&collator, this](const CompatibleFile &file1, const CompatibleFile &file2)
        {
            if (sortDescending)
                return collator.compare(file1.mimeType, file2.mimeType) > 0;
            else
                return collator.compare(file1.mimeType, file2.mimeType) < 0;
        });
    }
    else if (sortMode == 4) // Random
    {
        std::shuffle(currentFileDetails.folderFileInfoList.begin(), currentFileDetails.folderFileInfoList.end(), std::default_random_engine(randomSortSeed));
    }

    // Set current file index variable
    currentFileDetails.updateLoadedIndexInFolder();
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
    for (int i = currentFileDetails.loadedIndexInFolder-preloadingDistance; i <= currentFileDetails.loadedIndexInFolder+preloadingDistance; i++)
    {
        int index = i;

        // Don't try to cache the currently loaded image
        if (index == currentFileDetails.loadedIndexInFolder)
            continue;

        //keep within index range
        if (isLoopFoldersEnabled)
        {
            if (index > currentFileDetails.folderFileInfoList.length()-1)
                index = index-(currentFileDetails.folderFileInfoList.length());
            else if (index < 0)
                index = index+(currentFileDetails.folderFileInfoList.length());
        }

        //if still out of range after looping, just cancel the cache for this index
        if (index > currentFileDetails.folderFileInfoList.length()-1 || index < 0 || currentFileDetails.folderFileInfoList.isEmpty())
            continue;

        QString filePath = currentFileDetails.folderFileInfoList[index].absoluteFilePath;
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

    QFile imgFile(filePath);
    if (imgFile.size() > QPixmapCache::cacheLimit()/2)
        return;

    auto *cacheFutureWatcher = new QFutureWatcher<ReadData>();
    connect(cacheFutureWatcher, &QFutureWatcher<ReadData>::finished, this, [cacheFutureWatcher, this](){
        addToCache(cacheFutureWatcher->result());
        cacheFutureWatcher->deleteLater();
    });
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    cacheFutureWatcher->setFuture(QtConcurrent::run(this, &QVImageCore::readFile, filePath, true));
#else
    cacheFutureWatcher->setFuture(QtConcurrent::run(&QVImageCore::readFile, this, filePath, true));
#endif
}

void QVImageCore::addToCache(const ReadData &readData)
{
    if (readData.pixmap.isNull())
        return;

    QPixmapCache::insert(readData.fileInfo.absoluteFilePath(), readData.pixmap);

    auto *size = new qint64(readData.fileInfo.size());
    qvApp->setPreviouslyRecordedFileSize(readData.fileInfo.absoluteFilePath(), size);
    qvApp->setPreviouslyRecordedImageSize(readData.fileInfo.absoluteFilePath(), new QSize(readData.size));
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
    if (desiredSpeed < 0)
        desiredSpeed = 0;

    if (desiredSpeed > 1000)
        desiredSpeed = 1000;

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

QPixmap QVImageCore::scaleExpensively(const int desiredWidth, const int desiredHeight)
{
    return scaleExpensively(QSizeF(desiredWidth, desiredHeight));
}

QPixmap QVImageCore::scaleExpensively(const QSizeF desiredSize)
{
    if (!currentFileDetails.isPixmapLoaded)
        return QPixmap();

    QSize size = QSize(loadedPixmap.width(), loadedPixmap.height());
    size.scale(desiredSize.toSize(), Qt::KeepAspectRatio);

    // Get the current frame of the animation if this is an animation
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

    // If we are really close to the original size, just return the original
    if (abs(desiredSize.width() - relevantPixmap.width()) < 1 &&
        abs(desiredSize.height() - relevantPixmap.height()) < 1)
    {
        return relevantPixmap;
    }

    return relevantPixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);;
}


void QVImageCore::settingsUpdated()
{
    auto &settingsManager = qvApp->getSettingsManager();

    //loop folders
    isLoopFoldersEnabled = settingsManager.getBoolean("loopfoldersenabled");

    //preloading mode
    preloadingMode = settingsManager.getInteger("preloadingmode");
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
    sortMode = settingsManager.getInteger("sortmode");

    //sort ascending
    sortDescending = settingsManager.getBoolean("sortdescending");

    //allow mime content detection
    allowMimeContentDetection = settingsManager.getBoolean("allowmimecontentdetection");

    //update folder info to reflect new settings (e.g. sort order)
    updateFolderInfo();
}

void QVImageCore::FileDetails::updateLoadedIndexInFolder()
{
    const QString targetPath = fileInfo.absoluteFilePath();
    for (int i = 0; i < folderFileInfoList.length(); i++)
    {
        // Compare absoluteFilePath first because it's way faster, but double-check with
        // QFileInfo::operator== because it respects file system case sensitivity rules
        if (folderFileInfoList[i].absoluteFilePath.compare(targetPath, Qt::CaseInsensitive) == 0 &&
            QFileInfo(folderFileInfoList[i].absoluteFilePath) == fileInfo)
        {
            loadedIndexInFolder = i;
            return;
        }
    }
    loadedIndexInFolder = -1;
}
