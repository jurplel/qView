#include <QtTest>

#include "qvapplication.h"

#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QSettings>
#include <QTemporaryDir>

class ActionManagerTests : public QObject
{
    Q_OBJECT

public:
    ActionManagerTests();
    ~ActionManagerTests();

private slots:
    void testClonedActionsUntracked();
    void testNextFileWrapsPastMissingFiles();
    void testPreviousFileWrapsPastMissingFiles();
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

void ActionManagerTests::testNextFileWrapsPastMissingFiles()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString firstFile = tempDir.filePath("1.png");
    const QString currentFile = tempDir.filePath("2.png");
    const QString missingNextFile = tempDir.filePath("3.png");
    QVERIFY(QImage(2, 2, QImage::Format_ARGB32).save(firstFile));
    QVERIFY(QImage(3, 3, QImage::Format_ARGB32).save(currentFile));
    QVERIFY(QImage(4, 4, QImage::Format_ARGB32).save(missingNextFile));

    QWidget parent;
    QVGraphicsView view(&parent);
    view.loadFile(currentFile);
    QTRY_COMPARE(view.getCurrentFileDetails().fileInfo.absoluteFilePath(),
                 QFileInfo(currentFile).absoluteFilePath());

    QVERIFY(QFile::remove(missingNextFile));
    view.goToFile(QVGraphicsView::GoToFileMode::next);
    QTRY_COMPARE(view.getCurrentFileDetails().fileInfo.absoluteFilePath(),
                 QFileInfo(firstFile).absoluteFilePath());
}

void ActionManagerTests::testPreviousFileWrapsPastMissingFiles()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString missingPreviousFile = tempDir.filePath("1.png");
    const QString currentFile = tempDir.filePath("2.png");
    const QString lastFile = tempDir.filePath("3.png");
    QVERIFY(QImage(2, 2, QImage::Format_ARGB32).save(missingPreviousFile));
    QVERIFY(QImage(3, 3, QImage::Format_ARGB32).save(currentFile));
    QVERIFY(QImage(4, 4, QImage::Format_ARGB32).save(lastFile));

    QWidget parent;
    QVGraphicsView view(&parent);
    view.loadFile(currentFile);
    QTRY_COMPARE(view.getCurrentFileDetails().fileInfo.absoluteFilePath(),
                 QFileInfo(currentFile).absoluteFilePath());

    QVERIFY(QFile::remove(missingPreviousFile));
    view.goToFile(QVGraphicsView::GoToFileMode::previous);
    QTRY_COMPARE(view.getCurrentFileDetails().fileInfo.absoluteFilePath(),
                 QFileInfo(lastFile).absoluteFilePath());
}

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("qViewTests");
    QCoreApplication::setApplicationName("qViewTests");
    QSettings settings;
    settings.beginGroup("options");
    settings.setValue("colorspaceconversion", 0);
    settings.endGroup();

    QVApplication app(argc, argv);
    ActionManagerTests actionManagerTests;
    return QTest::qExec(&actionManagerTests, argc, argv);
}

#include "tst_actionmanagertests.moc"
