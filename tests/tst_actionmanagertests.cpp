#include <QtTest>

#include "qvapplication.h"

#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QTemporaryDir>

class ActionManagerTests : public QObject
{
    Q_OBJECT

public:
    ActionManagerTests();
    ~ActionManagerTests();

private slots:
    void testClonedActionsUntracked();
    void testInputPathSequence();
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

void ActionManagerTests::testInputPathSequence()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString firstFile = tempDir.filePath("1.png");
    const QString secondFile = tempDir.filePath("2.png");
    const QString unsupportedFile = tempDir.filePath("notes.txt");
    QVERIFY(QImage(2, 2, QImage::Format_ARGB32).save(firstFile));
    QVERIFY(QImage(3, 3, QImage::Format_ARGB32).save(secondFile));
    QVERIFY(QFile(unsupportedFile).open(QIODevice::WriteOnly));

    QVImageCore imageCore;
    QStringList warnings;
    const auto files = imageCore.getCompatibleFilesForInputs(
            { secondFile, tempDir.path(), unsupportedFile, tempDir.filePath("missing.png") },
            &warnings);

    QCOMPARE(files.length(), 2);
    QCOMPARE(files.at(0).absoluteFilePath, QFileInfo(secondFile).absoluteFilePath());
    QCOMPARE(files.at(1).absoluteFilePath, QFileInfo(firstFile).absoluteFilePath());
    QCOMPARE(warnings.length(), 2);
    QVERIFY(warnings.at(0).contains("unsupported"));
    QVERIFY(warnings.at(1).contains("missing"));
}

int main(int argc, char *argv[])
{
    QVApplication app(argc, argv);
    ActionManagerTests actionManagerTests;
    return QTest::qExec(&actionManagerTests, argc, argv);
}

#include "tst_actionmanagertests.moc"
