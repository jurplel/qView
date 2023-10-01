#ifndef QVIMAGECORE_H
#define QVIMAGECORE_H

#include "qvnamespace.h"
#include <QObject>
#include <QImageReader>
#include <QPixmap>
#include <QMovie>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QTimer>
#include <QCache>
#include <QElapsedTimer>

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#include <QColorSpace>
#else
typedef QString QColorSpace;
#endif

class QVImageCore : public QObject
{
    Q_OBJECT

public:
    struct CompatibleFile
    {
        QString absoluteFilePath;
        QString fileName;

        // Only populated if needed for sorting
        qint64 lastModified;
        qint64 lastCreated;
        qint64 size;
        QString mimeType;
    };

    struct FileDetails
    {
        QFileInfo fileInfo;
        QList<CompatibleFile> folderFileInfoList;
        int loadedIndexInFolder = -1;
        bool isLoadRequested = false;
        bool isPixmapLoaded = false;
        bool isMovieLoaded = false;
        QSize baseImageSize;
        QSize loadedPixmapSize;
        QElapsedTimer timeSinceLoaded;

        void updateLoadedIndexInFolder();
    };

    struct ReadData
    {
        QPixmap pixmap;
        QString absoluteFilePath;
        qint64 fileSize;
        QSize imageSize;
        QColorSpace targetColorSpace;
    };

    explicit QVImageCore(QObject *parent = nullptr);

    void loadFile(const QString &fileName, bool isReloading = false);
    ReadData readFile(const QString &fileName, const QColorSpace &targetColorSpace, bool forCache);
    void loadPixmap(const ReadData &readData);
    void closeImage();
    QList<CompatibleFile> getCompatibleFiles(const QString &dirPath);
    void sortCompatibleFiles(QList<CompatibleFile> &fileList);
    unsigned getRandomSortSeed(const QString &dirPath, const int fileCount);
    void updateFolderInfo(QString dirPath = QString());
    void requestCaching();
    void requestCachingFile(const QString &filePath, const QColorSpace &targetColorSpace);
    void addToCache(const ReadData &readImageAndFileInfo);
    static QString getPixmapCacheKey(const QString &absoluteFilePath, const qint64 &fileSize, const QColorSpace &targetColorSpace);
    QColorSpace getTargetColorSpace() const;
    QColorSpace detectDisplayColorSpace() const;

    void settingsUpdated();

    void jumpToNextFrame();
    void setPaused(bool desiredState);
    void setSpeed(int desiredSpeed);

    QPixmap scaleExpensively(const QSizeF desiredSize);

    //returned const reference is read-only
    const QPixmap& getLoadedPixmap() const {return loadedPixmap; }
    const QMovie& getLoadedMovie() const {return loadedMovie; }
    const FileDetails& getCurrentFileDetails() const {return currentFileDetails; }

signals:
    void animatedFrameChanged(QRect rect);

    void fileChanged();

    void readError(int errorNum, const QString &errorString, const QString &fileName);

private:
    QPixmap loadedPixmap;
    QMovie loadedMovie;

    FileDetails currentFileDetails;

    QFutureWatcher<ReadData> loadFutureWatcher;

    bool isLoopFoldersEnabled {true};
    Qv::PreloadMode preloadingMode {Qv::PreloadMode::Adjacent};
    Qv::SortMode sortMode {Qv::SortMode::Name};
    bool sortDescending {false};
    bool allowMimeContentDetection {false};
    Qv::ColorSpaceConversion colorSpaceConversion {Qv::ColorSpaceConversion::AutoDetect};

    static QCache<QString, ReadData> pixmapCache;

    quint32 baseRandomSortSeed {static_cast<quint32>(std::chrono::system_clock::now().time_since_epoch().count())};

    QStringList lastFilesPreloaded;

    int largestDimension {0};

    bool waitingOnLoad {false};
};

#endif // QVIMAGECORE_H
