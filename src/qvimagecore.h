#ifndef QVIMAGECORE_H
#define QVIMAGECORE_H

#include <QObject>
#include <QImageReader>
#include <QPixmap>
#include <QMovie>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QTimer>
#include <QCache>
#include <QElapsedTimer>

class QVImageCore : public QObject
{
    Q_OBJECT

public:
    struct FileDetails
    {
        QFileInfo fileInfo;
        QFileInfoList folderFileInfoList;
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
        QFileInfo fileInfo;
        QSize size;
    };

    explicit QVImageCore(QObject *parent = nullptr);

    void loadFile(const QString &fileName);
    ReadData readFile(const QString &fileName, bool forCache);
    void loadPixmap(const ReadData &readData, bool fromCache);
    void closeImage();
    QFileInfoList getCompatibleFiles();
    void updateFolderInfo();
    void requestCaching();
    void requestCachingFile(const QString &filePath);
    void addToCache(const ReadData &readImageAndFileInfo);

    void settingsUpdated();

    void jumpToNextFrame();
    void setPaused(bool desiredState);
    void setSpeed(int desiredSpeed);

    void rotateImage(int rotation);
    QImage matchCurrentRotation(const QImage &imageToRotate);
    QPixmap matchCurrentRotation(const QPixmap &pixmapToRotate);

    QPixmap scaleExpensively(const int desiredWidth, const int desiredHeight);
    QPixmap scaleExpensively(const QSizeF desiredSize);

    //returned const reference is read-only
    const QPixmap& getLoadedPixmap() const {return loadedPixmap; }
    const QMovie& getLoadedMovie() const {return loadedMovie; }
    const FileDetails& getCurrentFileDetails() const {return currentFileDetails; }
    int getCurrentRotation() const {return currentRotation; }

signals:
    void animatedFrameChanged(QRect rect);

    void updateLoadedPixmapItem();

    void fileChanged();

    void readError(int errorNum, const QString &errorString, const QString &fileName);

private:
    QPixmap loadedPixmap;
    QMovie loadedMovie;

    FileDetails currentFileDetails;
    int currentRotation;

    QFutureWatcher<ReadData> loadFutureWatcher;

    bool isLoopFoldersEnabled;
    int preloadingMode;
    int sortMode;
    bool sortDescending;

    QPair<QString, uint> lastDirInfo;
    unsigned randomSortSeed;

    QStringList lastFilesPreloaded;

    int largestDimension;

    bool waitingOnLoad;
};

#endif // QVIMAGECORE_H
