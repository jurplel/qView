#include "qvimagecore.h"
#include "qvapplication.h"
#include "qvwin32functions.h"
#include "qvcocoafunctions.h"
#include "qvlinuxx11functions.h"
#include <random>
#include <QMessageBox>
#include <QDir>
#include <QUrl>
#include <QSettings>
#include <QCollator>
#include <QtConcurrent/QtConcurrentRun>
#include <QIcon>
#include <QGuiApplication>
#include <QScreen>

QCache<QString, QVImageCore::ReadData> QVImageCore::pixmapCache;

QVImageCore::QVImageCore(QObject *parent) : QObject(parent)
{
// Set allocation limit to 8 GiB on Qt6
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QImageReader::setAllocationLimit(8192);
#endif

    isLoopFoldersEnabled = true;
    preloadingMode = Qv::PreloadMode::Adjacent;
    sortMode = Qv::SortMode::Name;
    sortDescending = false;
    allowMimeContentDetection = false;
    colorSpaceConversion = Qv::ColorSpaceConversion::AutoDetect;

    baseRandomSortSeed = std::chrono::system_clock::now().time_since_epoch().count();

    connect(&loadedMovie, &QMovie::updated, this, &QVImageCore::animatedFrameChanged);

    connect(&loadFutureWatcher, &QFutureWatcher<ReadData>::finished, this, [this](){
        loadPixmap(loadFutureWatcher.result());
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

void QVImageCore::loadFile(const QString &fileName, bool isReloading)
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

    QColorSpace targetColorSpace = getTargetColorSpace();
    QString cacheKey = getPixmapCacheKey(sanitaryFileName, fileInfo.size(), targetColorSpace);

    //check if cached already before loading the long way
    auto *cachedData = isReloading ? nullptr : QVImageCore::pixmapCache.take(cacheKey);
    if (cachedData != nullptr)
    {
        ReadData readData = *cachedData;
        delete cachedData;
        loadPixmap(readData);
    }
    else
    {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        loadFutureWatcher.setFuture(QtConcurrent::run(this, &QVImageCore::readFile, sanitaryFileName, targetColorSpace, false));
#else
        loadFutureWatcher.setFuture(QtConcurrent::run(&QVImageCore::readFile, this, sanitaryFileName, targetColorSpace, false));
#endif
    }
}

QVImageCore::ReadData QVImageCore::readFile(const QString &fileName, const QColorSpace &targetColorSpace, bool forCache)
{
    QImageReader imageReader;
    imageReader.setDecideFormatFromContent(true);
    imageReader.setAutoTransform(true);

    imageReader.setFileName(fileName);

    QImage readImage;
    if (imageReader.format() == "svg" || imageReader.format() == "svgz")
    {
        // Render vectors into a high resolution
        QIcon icon;
        icon.addFile(fileName);
        readImage = icon.pixmap(largestDimension).toImage();
        // If this fails, try reading the normal way so that a proper error message is given
        if (readImage.isNull())
            readImage = imageReader.read();
    }
    else
    {
        readImage = imageReader.read();
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    // Assume image is sRGB if it doesn't specify
    if (!readImage.colorSpace().isValid())
        readImage.setColorSpace(QColorSpace::SRgb);

    // Convert image color space if we have a target that's different
    if (targetColorSpace.isValid() && readImage.colorSpace() != targetColorSpace)
        readImage.convertToColorSpace(targetColorSpace);
#endif

    QPixmap readPixmap = QPixmap::fromImage(readImage);
    QFileInfo fileInfo(fileName);

    ReadData readData = {
        readPixmap,
        fileInfo.absoluteFilePath(),
        fileInfo.size(),
        imageReader.size(),
        targetColorSpace
    };
    // Only error out when not loading for cache
    if (readPixmap.isNull() && !forCache)
    {
        emit readError(imageReader.error(), imageReader.errorString(), fileInfo.fileName());
    }

    return readData;
}

void QVImageCore::loadPixmap(const ReadData &readData)
{
    // Do this first so we can keep folder info even when loading errored files
    currentFileDetails.fileInfo = QFileInfo(readData.absoluteFilePath);
    currentFileDetails.updateLoadedIndexInFolder();
    if (currentFileDetails.loadedIndexInFolder == -1)
        updateFolderInfo();

    // Reset mechanism to avoid stalling while loading
    waitingOnLoad = false;

    if (readData.pixmap.isNull())
        return;

    loadedPixmap = readData.pixmap;

    // Set file details
    currentFileDetails.isPixmapLoaded = true;
    currentFileDetails.baseImageSize = readData.imageSize;
    currentFileDetails.loadedPixmapSize = loadedPixmap.size();
    if (currentFileDetails.baseImageSize == QSize(-1, -1))
    {
        qInfo() << "QImageReader::size gave an invalid size for " + currentFileDetails.fileInfo.fileName() + ", using size from loaded pixmap";
        currentFileDetails.baseImageSize = currentFileDetails.loadedPixmapSize;
    }

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

    if (loadedMovie.isValid() && loadedMovie.frameCount() != 1)
        loadedMovie.start();

    currentFileDetails.isMovieLoaded = loadedMovie.state() == QMovie::Running;

    if (!currentFileDetails.isMovieLoaded)
        if (auto device = loadedMovie.device())
            device->close();
    
    currentFileDetails.timeSinceLoaded.start();

    emit fileChanged();

    requestCaching();
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
QList<QVImageCore::CompatibleFile> QVImageCore::getCompatibleFiles(const QString &dirPath)
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
        if (!matched || sortMode == Qv::SortMode::Type)
        {
            mimeType = mimeDb.mimeTypeForFile(absoluteFilePath, mimeMatchMode).name();
            matched |= mimeTypes.contains(mimeType);
        }
        if (matched)
        {
            fileList.append({
                absoluteFilePath,
                fileName,
                sortMode == Qv::SortMode::DateModified ? fileInfo.lastModified().toMSecsSinceEpoch() : 0,
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
                sortMode == Qv::SortMode::DateCreated ? fileInfo.birthTime().toMSecsSinceEpoch() : 0,
#else
                sortMode == Qv::SortMode::DateCreated ? fileInfo.created().toMSecsSinceEpoch() : 0,
#endif
                sortMode == Qv::SortMode::Size ? fileInfo.size() : 0,
                sortMode == Qv::SortMode::Type ? mimeType : QString()
            });
        }
    }

    return fileList;
}

void QVImageCore::sortCompatibleFiles(QList<CompatibleFile> &fileList)
{
    if (sortMode == Qv::SortMode::Name)
    {
        QCollator collator;
        collator.setNumericMode(true);
        std::sort(fileList.begin(),
                  fileList.end(),
                  [&collator, this](const CompatibleFile &file1, const CompatibleFile &file2)
        {
            if (sortDescending)
                return collator.compare(file1.fileName, file2.fileName) > 0;
            else
                return collator.compare(file1.fileName, file2.fileName) < 0;
        });
    }
    else if (sortMode == Qv::SortMode::DateModified)
    {
        std::sort(fileList.begin(),
                  fileList.end(),
                  [this](const CompatibleFile &file1, const CompatibleFile &file2)
        {
            if (sortDescending)
                return file1.lastModified < file2.lastModified;
            else
                return file1.lastModified > file2.lastModified;
        });
    }
    else if (sortMode == Qv::SortMode::DateCreated)
    {
        std::sort(fileList.begin(),
                  fileList.end(),
                  [this](const CompatibleFile &file1, const CompatibleFile &file2)
        {
            if (sortDescending)
                return file1.lastCreated < file2.lastCreated;
            else
                return file1.lastCreated > file2.lastCreated;
        });

    }
    else if (sortMode == Qv::SortMode::Size)
    {
        std::sort(fileList.begin(),
                  fileList.end(),
                  [this](const CompatibleFile &file1, const CompatibleFile &file2)
        {
            if (sortDescending)
                return file1.size < file2.size;
            else
                return file1.size > file2.size;
        });
    }
    else if (sortMode == Qv::SortMode::Type)
    {
        QCollator collator;
        std::sort(fileList.begin(),
                  fileList.end(),
                  [&collator, this](const CompatibleFile &file1, const CompatibleFile &file2)
        {
            if (sortDescending)
                return collator.compare(file1.mimeType, file2.mimeType) > 0;
            else
                return collator.compare(file1.mimeType, file2.mimeType) < 0;
        });
    }
    else if (sortMode == Qv::SortMode::Random)
    {
        unsigned randomSortSeed = getRandomSortSeed(QFileInfo(fileList.value(0).absoluteFilePath).path(), fileList.count());
        std::shuffle(fileList.begin(), fileList.end(), std::default_random_engine(randomSortSeed));
    }
}

unsigned QVImageCore::getRandomSortSeed(const QString &dirPath, const int fileCount)
{
    QString seed = QString::number(baseRandomSortSeed, 16) + dirPath + QString::number(fileCount, 16);
    QByteArray hash = QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Md5);
    return hash.toHex().left(8).toUInt(nullptr, 16);
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

    // Get file listing
    currentFileDetails.folderFileInfoList = getCompatibleFiles(dirPath);

    // Sorting
    sortCompatibleFiles(currentFileDetails.folderFileInfoList);

    // Set current file index variable
    currentFileDetails.updateLoadedIndexInFolder();
}

void QVImageCore::requestCaching()
{
    if (preloadingMode == Qv::PreloadMode::Disabled)
    {
        QVImageCore::pixmapCache.clear();
        return;
    }

    QColorSpace targetColorSpace = getTargetColorSpace();

    int preloadingDistance = preloadingMode == Qv::PreloadMode::Extended ? 4 : 1;

    QStringList filesToPreload;
    for (int i = currentFileDetails.loadedIndexInFolder-preloadingDistance; i <= currentFileDetails.loadedIndexInFolder+preloadingDistance; i++)
    {
        int index = i;

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

        // Don't try to cache the currently loaded image
        if (index == currentFileDetails.loadedIndexInFolder)
            continue;

        requestCachingFile(filePath, targetColorSpace);
    }
    lastFilesPreloaded = filesToPreload;

}

void QVImageCore::requestCachingFile(const QString &filePath, const QColorSpace &targetColorSpace)
{
    QFile imgFile(filePath);
    QString cacheKey = getPixmapCacheKey(filePath, imgFile.size(), targetColorSpace);

    //check if image is already loaded or requested
    if (QVImageCore::pixmapCache.contains(cacheKey) || lastFilesPreloaded.contains(filePath))
        return;

    if (imgFile.size()/1024 > QVImageCore::pixmapCache.maxCost()/2)
        return;

    auto *cacheFutureWatcher = new QFutureWatcher<ReadData>();
    connect(cacheFutureWatcher, &QFutureWatcher<ReadData>::finished, this, [cacheFutureWatcher, this](){
        addToCache(cacheFutureWatcher->result());
        cacheFutureWatcher->deleteLater();
    });

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    cacheFutureWatcher->setFuture(QtConcurrent::run(this, &QVImageCore::readFile, filePath, targetColorSpace, true));
#else
    cacheFutureWatcher->setFuture(QtConcurrent::run(&QVImageCore::readFile, this, filePath, targetColorSpace, true));
#endif
}

void QVImageCore::addToCache(const ReadData &readData)
{
    if (readData.pixmap.isNull())
        return;

    QString cacheKey = getPixmapCacheKey(readData.absoluteFilePath, readData.fileSize, readData.targetColorSpace);
    qint64 pixmapMemoryBytes = static_cast<qint64>(readData.pixmap.width()) * readData.pixmap.height() * readData.pixmap.depth() / 8;

    QVImageCore::pixmapCache.insert(cacheKey, new ReadData(readData), qMax(pixmapMemoryBytes / 1024, 1LL));
}

QString QVImageCore::getPixmapCacheKey(const QString &absoluteFilePath, const qint64 &fileSize, const QColorSpace &targetColorSpace)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QString targetColorSpaceHash = QCryptographicHash::hash(targetColorSpace.iccProfile(), QCryptographicHash::Md5).toHex();
#else
    QString targetColorSpaceHash = "";
#endif
    return absoluteFilePath + "\n" + QString::number(fileSize) + "\n" + targetColorSpaceHash;
}

QColorSpace QVImageCore::getTargetColorSpace() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return
        colorSpaceConversion == Qv::ColorSpaceConversion::AutoDetect ? detectDisplayColorSpace() :
        colorSpaceConversion == Qv::ColorSpaceConversion::SRgb ? QColorSpace::SRgb :
        colorSpaceConversion == Qv::ColorSpaceConversion::DisplayP3 ? QColorSpace::DisplayP3 :
        QColorSpace();
#else
    return {};
#endif
}

QColorSpace QVImageCore::detectDisplayColorSpace() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QWindow *window = static_cast<QWidget*>(parent())->window()->windowHandle();

    QByteArray profileData;
#ifdef WIN32_LOADED
    profileData = QVWin32Functions::getIccProfileForWindow(window);
#endif
#ifdef COCOA_LOADED
    profileData = QVCocoaFunctions::getIccProfileForWindow(window);
#endif
#ifdef X11_LOADED
    profileData = QVLinuxX11Functions::getIccProfileForWindow(window);
#endif

    if (!profileData.isEmpty())
        return QColorSpace::fromIccProfile(profileData);
#endif

    return {};
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

QPixmap QVImageCore::scaleExpensively(const QSizeF desiredSize)
{
    if (!currentFileDetails.isPixmapLoaded)
        return QPixmap();

    // Get the current frame of the animation if this is an animation
    QPixmap relevantPixmap;
    if (!currentFileDetails.isMovieLoaded)
    {
        relevantPixmap = loadedPixmap;
    }
    else
    {
        relevantPixmap = loadedMovie.currentPixmap();
    }

    // If we are really close to the original size, just return the original
    if (abs(desiredSize.width() - relevantPixmap.width()) < 1 &&
        abs(desiredSize.height() - relevantPixmap.height()) < 1)
    {
        return relevantPixmap;
    }

    QSize size = desiredSize.toSize();
    size.rwidth() = qMax(size.width(), 1);
    size.rheight() = qMax(size.height(), 1);

    return relevantPixmap.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}


void QVImageCore::settingsUpdated()
{
    auto &settingsManager = qvApp->getSettingsManager();

    //loop folders
    isLoopFoldersEnabled = settingsManager.getBoolean("loopfoldersenabled");

    //preloading mode
    preloadingMode = settingsManager.getEnum<Qv::PreloadMode>("preloadingmode");
    switch (preloadingMode) {
    case Qv::PreloadMode::Adjacent:
    {
        QVImageCore::pixmapCache.setMaxCost(204800);
        break;
    }
    case Qv::PreloadMode::Extended:
    {
        QVImageCore::pixmapCache.setMaxCost(819200);
        break;
    }
    default:
        break;
    }

    //sort mode
    sortMode = settingsManager.getEnum<Qv::SortMode>("sortmode");

    //sort ascending
    sortDescending = settingsManager.getBoolean("sortdescending");

    //allow mime content detection
    allowMimeContentDetection = settingsManager.getBoolean("allowmimecontentdetection");

    //update folder info to reflect new settings (e.g. sort order)
    updateFolderInfo();

    //colorspaceconversion
    Qv::ColorSpaceConversion oldColorSpaceConversion = colorSpaceConversion;
    colorSpaceConversion = settingsManager.getEnum<Qv::ColorSpaceConversion>("colorspaceconversion");

    if (colorSpaceConversion != oldColorSpaceConversion && currentFileDetails.isPixmapLoaded)
        loadFile(currentFileDetails.fileInfo.absoluteFilePath());
}

void QVImageCore::FileDetails::updateLoadedIndexInFolder()
{
    const QString targetPath = fileInfo.absoluteFilePath().normalized(QString::NormalizationForm_D);
    for (int i = 0; i < folderFileInfoList.length(); i++)
    {
        // Compare absoluteFilePath first because it's way faster, but double-check with
        // QFileInfo::operator== because it respects file system case sensitivity rules
        QString candidatePath = folderFileInfoList[i].absoluteFilePath.normalized(QString::NormalizationForm_D);
        if (candidatePath.compare(targetPath, Qt::CaseInsensitive) == 0 &&
            QFileInfo(folderFileInfoList[i].absoluteFilePath) == fileInfo)
        {
            loadedIndexInFolder = i;
            return;
        }
    }
    loadedIndexInFolder = -1;
}
