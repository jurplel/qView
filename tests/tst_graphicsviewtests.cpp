#include <QtTest>

#include <QGraphicsScene>
#include <QImage>
#include <QSettings>
#include <QTemporaryDir>

#include "qvapplication.h"
#include "qvgraphicsview.h"

namespace {

constexpr int IMAGE_EDGE = 800;
constexpr int SMALL_VIEW_EDGE = 300;
constexpr int LARGE_VIEW_EDGE = 600;
constexpr int LOAD_TIMEOUT_MS = 10000;

// The view swaps between a full resolution pixmap and a pre-scaled one as it
// renders, so the item's own size is not stable. What the user actually sees is,
// hence measuring the item through the view transform.
QSizeF displayedImageSize(const QVGraphicsView &view)
{
    return view.transform().mapRect(view.scene()->itemsBoundingRect()).size();
}

// MARGIN is negative so a fitted image overhangs the viewport by a couple of
// pixels on each side.
qreal fittedWidthLimit(const QVGraphicsView &view)
{
    return view.viewport()->width() + 4;
}

}

class GraphicsViewTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void resizeRefitsImageInFitMode();
    void resizeRefitsImageAfterOriginalSize();

private:
    void showViewWithImage(QVGraphicsView &view);

    QTemporaryDir imageDir;
    QString imagePath;
};

void GraphicsViewTests::initTestCase()
{
    QVERIFY(imageDir.isValid());

    QImage image(IMAGE_EDGE, IMAGE_EDGE, QImage::Format_RGB32);
    for (int y = 0; y < IMAGE_EDGE; y++) {
        for (int x = 0; x < IMAGE_EDGE; x++)
            image.setPixel(x, y, qRgb(x % 256, y % 256, (x + y) % 256));
    }

    imagePath = imageDir.filePath("square.png");
    QVERIFY(image.save(imagePath));
}

void GraphicsViewTests::showViewWithImage(QVGraphicsView &view)
{
    view.resize(SMALL_VIEW_EDGE, SMALL_VIEW_EDGE);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    // A cached image loads synchronously, so poll instead of waiting on the signal
    QSignalSpy pixmapUpdated(&view, &QVGraphicsView::updatedLoadedPixmapItem);
    view.loadFile(imagePath);
    QTRY_VERIFY_WITH_TIMEOUT(!pixmapUpdated.isEmpty(), LOAD_TIMEOUT_MS);
    QVERIFY(view.getCurrentFileDetails().isPixmapLoaded);
}

void GraphicsViewTests::resizeRefitsImageInFitMode()
{
    QVGraphicsView view;
    showViewWithImage(view);

    const qreal fittedWidth = displayedImageSize(view).width();
    QVERIFY(fittedWidth < IMAGE_EDGE);

    view.resize(LARGE_VIEW_EDGE, LARGE_VIEW_EDGE);

    QTRY_VERIFY(displayedImageSize(view).width() > fittedWidth);
    QVERIFY(displayedImageSize(view).width() <= fittedWidthLimit(view));
}

void GraphicsViewTests::resizeRefitsImageAfterOriginalSize()
{
    QVGraphicsView view;
    showViewWithImage(view);

    const qreal fittedWidth = displayedImageSize(view).width();

    view.originalSize();
    QCOMPARE(displayedImageSize(view), QSizeF(IMAGE_EDGE, IMAGE_EDGE));

    view.resize(LARGE_VIEW_EDGE, LARGE_VIEW_EDGE);

    // Original size is a one-shot, so the resize has to fit the image again
    QTRY_VERIFY(displayedImageSize(view).width() <= fittedWidthLimit(view));
    QVERIFY(displayedImageSize(view).width() > fittedWidth);
}

int main(int argc, char *argv[])
{
    // Keep the test off the real user's settings, which would otherwise decide
    // crop mode and scaling behaviour for us.
    QTemporaryDir settingsDir;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());

    QVApplication app(argc, argv);
    GraphicsViewTests graphicsViewTests;
    return QTest::qExec(&graphicsViewTests, argc, argv);
}

#include "tst_graphicsviewtests.moc"
