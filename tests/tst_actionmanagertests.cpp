#include <QtTest>

#include "qvapplication.h"
#include "qvexifpanel.h"

#include <exiv2/exiv2.hpp>

#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtEndian>

namespace {
bool createImageWithExif(const QString &filePath, const std::string &cameraMake)
{
    QImage image(16, 16, QImage::Format_RGB32);
    image.fill(Qt::blue);
    if (!image.save(filePath, "JPEG"))
        return false;

    try {
        auto exifImage = Exiv2::ImageFactory::open(QFile::encodeName(filePath).toStdString());
        exifImage->readMetadata();
        exifImage->exifData()["Exif.Image.Make"] = cameraMake;
        exifImage->exifData()["Exif.Image.Model"] = "qView test camera";
        exifImage->exifData()["Exif.Photo.ISOSpeedRatings"] = "100";
        exifImage->writeMetadata();
    } catch (const std::exception &) {
        return false;
    }
    return true;
}

QString exifValue(QTreeWidget *treeWidget, const QString &key)
{
    const auto items = treeWidget->findItems(key, Qt::MatchExactly, 0);
    return items.isEmpty() ? QString() : items.first()->text(1);
}
} // namespace

class ActionManagerTests : public QObject
{
    Q_OBJECT

public:
    ActionManagerTests();
    ~ActionManagerTests();

private slots:
    void testClonedActionsUntracked();
    void testExifPanelReadsEveryEntry();
    void testExifPanelReadsPngExif();
    void testExifPanelTabToggleAndNavigation();
};

ActionManagerTests::ActionManagerTests() { }

ActionManagerTests::~ActionManagerTests() { }

void ActionManagerTests::testClonedActionsUntracked()
{
    // Get initial counts of certain actions
    int fullscreenCount = qvApp->getActionManager().getAllInstancesOfAction("fullscreen").length();
    int openCount = qvApp->getActionManager().getAllInstancesOfAction("open").length();
    qDebug() << fullscreenCount;

    // Have window clone actions
    MainWindow window;
    window.show();
    // Make sure they were cloned
    QVERIFY(qvApp->getActionManager().getAllInstancesOfAction("fullscreen").length()
            != fullscreenCount);
    QVERIFY(qvApp->getActionManager().getAllInstancesOfAction("open").length() != openCount);
    // Untrack them
    window.close();

    // Make sure the count has not changed from the initial
    QCOMPARE(qvApp->getActionManager().getAllInstancesOfAction("fullscreen").length(),
             fullscreenCount);
    QCOMPARE(qvApp->getActionManager().getAllInstancesOfAction("open").length(), openCount);
}

void ActionManagerTests::testExifPanelReadsEveryEntry()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString sourcePath = temporaryDirectory.filePath("metadata.jpg");
    const QString filePath = temporaryDirectory.filePath("metadata-相机-😀.jpg");
    QVERIFY(createImageWithExif(sourcePath, "First Camera"));
    QVERIFY(QFile::rename(sourcePath, filePath));

    QVExifPanel panel;
    panel.setFilePath(filePath);

    auto *treeWidget = panel.findChild<QTreeWidget *>("exifTreeWidget");
    QVERIFY(treeWidget);
    QVERIFY(!treeWidget->isHidden());

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray fileData = file.readAll();
    const auto *payload = reinterpret_cast<const Exiv2::byte *>(fileData.constData());
    auto exifImage = Exiv2::ImageFactory::open(payload, static_cast<size_t>(fileData.size()));
    exifImage->readMetadata();
    QCOMPARE(treeWidget->topLevelItemCount(), static_cast<int>(exifImage->exifData().count()));
    QCOMPARE(exifValue(treeWidget, "Exif.Image.Make"), QString("First Camera"));
    QCOMPARE(exifValue(treeWidget, "Exif.Image.Model"), QString("qView test camera"));
    QCOMPARE(exifValue(treeWidget, "Exif.Photo.ISOSpeedRatings"), QString("100"));
}

void ActionManagerTests::testExifPanelReadsPngExif()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString filePath = temporaryDirectory.filePath("metadata.png");
    QImage image(16, 16, QImage::Format_RGB32);
    image.fill(Qt::green);
    QVERIFY(image.save(filePath, "PNG"));

    Exiv2::ExifData exifData;
    exifData["Exif.Image.Make"] = "PNG Camera";
    exifData["Exif.Image.Model"] = "qView PNG test";
    Exiv2::Blob exifBlob;
    Exiv2::ExifParser::encode(exifBlob, Exiv2::littleEndian, exifData);
    const QByteArray payload(reinterpret_cast<const char *>(exifBlob.data()),
                             static_cast<int>(exifBlob.size()));

    QByteArray lengthBytes(4, 0);
    qToBigEndian<quint32>(static_cast<quint32>(payload.size()),
                          reinterpret_cast<uchar *>(lengthBytes.data()));
    const QByteArray typeAndPayload = QByteArray("eXIf") + payload;

    quint32 crc = 0xffffffff;
    for (const char byte : typeAndPayload) {
        crc ^= static_cast<quint8>(byte);
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
    }
    crc = ~crc;

    QByteArray crcBytes(4, 0);
    qToBigEndian<quint32>(crc, reinterpret_cast<uchar *>(crcBytes.data()));
    const QByteArray exifChunk = lengthBytes + typeAndPayload + crcBytes;

    QFile pngFile(filePath);
    QVERIFY(pngFile.open(QIODevice::ReadOnly));
    QByteArray pngData = pngFile.readAll();
    pngFile.close();

    const int endChunkTypePosition = pngData.lastIndexOf("IEND");
    QVERIFY(endChunkTypePosition >= 4);
    pngData.insert(endChunkTypePosition - 4, exifChunk);
    QVERIFY(pngFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(pngFile.write(pngData), static_cast<qint64>(pngData.size()));
    pngFile.close();

    QVExifPanel panel;
    panel.setFilePath(filePath);
    auto *treeWidget = panel.findChild<QTreeWidget *>("exifTreeWidget");
    QVERIFY(treeWidget);
    QVERIFY(!treeWidget->isHidden());
    QCOMPARE(exifValue(treeWidget, "Exif.Image.Make"), QString("PNG Camera"));
    QCOMPARE(exifValue(treeWidget, "Exif.Image.Model"), QString("qView PNG test"));
}

void ActionManagerTests::testExifPanelTabToggleAndNavigation()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString firstPath = temporaryDirectory.filePath("first.jpg");
    const QString secondPath = temporaryDirectory.filePath("second.jpg");
    QVERIFY(createImageWithExif(firstPath, "First Camera"));
    QVERIFY(createImageWithExif(secondPath, "Second Camera"));

    MainWindow window;
    window.openFile(firstPath);
    QTRY_VERIFY_WITH_TIMEOUT(window.getCurrentFileDetails().isPixmapLoaded, 15000);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.activateWindow();
    QTRY_VERIFY_WITH_TIMEOUT(window.isActiveWindow(), 2000);

    auto *panel = window.findChild<QVExifPanel *>("exifPanel");
    QVERIFY(panel);
    auto *treeWidget = panel->findChild<QTreeWidget *>("exifTreeWidget");
    QVERIFY(treeWidget);
    QVERIFY(panel->isHidden());

    QTest::keyClick(&window, Qt::Key_Tab);
    QTRY_VERIFY_WITH_TIMEOUT(!panel->isHidden(), 2000);
    QTRY_COMPARE_WITH_TIMEOUT(exifValue(treeWidget, "Exif.Image.Make"), QString("First Camera"),
                              2000);

    QTest::keyClick(&window, Qt::Key_Right);
    QTRY_COMPARE_WITH_TIMEOUT(window.getCurrentFileDetails().fileInfo.absoluteFilePath(),
                              secondPath, 15000);
    QVERIFY(!panel->isHidden());
    QTRY_COMPARE_WITH_TIMEOUT(exifValue(treeWidget, "Exif.Image.Make"), QString("Second Camera"),
                              2000);

    QTest::keyClick(&window, Qt::Key_Left);
    QTRY_COMPARE_WITH_TIMEOUT(window.getCurrentFileDetails().fileInfo.absoluteFilePath(), firstPath,
                              15000);
    QVERIFY(!panel->isHidden());
    QTRY_COMPARE_WITH_TIMEOUT(exifValue(treeWidget, "Exif.Image.Make"), QString("First Camera"),
                              2000);

    QTest::keyClick(&window, Qt::Key_Tab);
    QTRY_VERIFY_WITH_TIMEOUT(panel->isHidden(), 2000);
}

int main(int argc, char *argv[])
{
    QVApplication app(argc, argv);
    ActionManagerTests actionManagerTests;
    return QTest::qExec(&actionManagerTests, argc, argv);
}

#include "tst_actionmanagertests.moc"
