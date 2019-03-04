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

class QVImageCore : public QObject
{
    Q_OBJECT

public:
    enum class scaleMode
    {
       normal,
       width,
       height
    };
    Q_ENUM(scaleMode)

    struct QVFileDetails
    {
        QFileInfo fileInfo;
        bool isPixmapLoaded;
        bool isMovieLoaded;
        QFileInfoList folder;
        int folderIndex;
        QSize imageSize;
        QSize loadedPixmapSize;
    };

    struct QVImageAndFileInfo
    {
        QImage readImage;
        QFileInfo readFileInfo;
    };

    explicit QVImageCore(QObject *parent = nullptr);

    void loadFile(const QString &fileName);
    QVImageAndFileInfo readFile(const QString &fileName);
    void postRead(QImage loadedImage);
    void postLoad();
    void updateFolderInfo();
    void requestCaching();
    void requestCachingFile(const QString &filePath);
    void addToCache(QVImageAndFileInfo loadedImageAndFileInfo);

    void loadSettings();

    void jumpToNextFrame();
    void setPaused(bool desiredState);
    void setSpeed(int desiredSpeed);

    void rotateImage(int rotation);
    const QImage matchCurrentRotation(const QImage &imageToRotate);
    const QPixmap matchCurrentRotation(const QPixmap &pixmapToRotate);

    const QPixmap scaleExpensively(const int desiredWidth, const int desiredHeight, const scaleMode mode = scaleMode::normal);
    const QPixmap scaleExpensively(const QSize desiredSize, const scaleMode mode = scaleMode::normal);

    //returned const reference is read-only
    const QPixmap& getLoadedPixmap() const {return loadedPixmap; }
    const QMovie& getLoadedMovie() const {return loadedMovie; }
    const QVFileDetails& getCurrentFileDetails() const {return currentFileDetails; }
    int getCurrentRotation() const {return currentRotation; }


signals:
    void animatedFrameChanged(QRect rect);

    void fileInfoUpdated();

    void updateLoadedPixmapItem();

    void fileRead();

    void readError(const QString &errorString, const QString fileName);

private:
    const QStringList filterList = (QStringList() << "*.bmp" << "*.cur" << "*.gif" << "*.icns" << "*.ico" << "*.jp2" << "*.jpeg" << "*.jpe" << "*.jpg" << "*.mng" << "*.pbm" << "*.pgm" << "*.png" << "*.ppm" << "*.svg" << "*.svgz" << "*.tif" << "*.tiff" << "*.wbmp" << "*.webp" << "*.xbm" << "*.xpm");

    QPixmap loadedPixmap;
    QMovie loadedMovie;
    QImageReader imageReader;

    QVFileDetails currentFileDetails;
    QVFileDetails lastFileDetails;
    int currentRotation;

    QFutureWatcher<QVImageAndFileInfo> loadFutureWatcher;

    bool justLoadedFromCache;
    QCache<QString, qint64> previouslyRecordedFileSizes;

    bool isLoopFoldersEnabled;

    int preloadingMode;
    QStringList lastFilesPreloaded;

    QTimer *fileChangeRateTimer;
};

#endif // QVIMAGECORE_H
