#include <QtTest>

#include "qvapplication.h"
#include "qvipcserver.h"

class ActionManagerTests : public QObject
{
    Q_OBJECT

public:
    ActionManagerTests();
    ~ActionManagerTests();

private slots:
    void testClonedActionsUntracked();
    void testIpcServerOptionDisabledByDefault();
    void testIpcServerOptionUsesDefaultSocket();
    void testIpcServerOptionAcceptsExplicitSocket();
    void testIpcCurrentFilePathResponse();
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

void ActionManagerTests::testIpcServerOptionDisabledByDefault()
{
    QStringList arguments = { "qview", "image.jpg" };
    const auto option = QVIPCServer::extractServerOption(&arguments);

    QVERIFY(!option.enabled);
    QVERIFY(option.serverName.isEmpty());
    QCOMPARE(arguments, QStringList({ "qview", "image.jpg" }));
}

void ActionManagerTests::testIpcServerOptionUsesDefaultSocket()
{
    QStringList arguments = { "qview", "image.jpg", "--ipc-server" };
    const auto option = QVIPCServer::extractServerOption(&arguments);

    QVERIFY(option.enabled);
    QCOMPARE(option.serverName, QVIPCServer::defaultServerName());
    QCOMPARE(arguments, QStringList({ "qview", "image.jpg" }));
}

void ActionManagerTests::testIpcServerOptionAcceptsExplicitSocket()
{
    QStringList arguments = { "qview", "--ipc-server=/tmp/qview-test.sock", "image.jpg" };
    const auto option = QVIPCServer::extractServerOption(&arguments);

    QVERIFY(option.enabled);
    QCOMPARE(option.serverName, QString("/tmp/qview-test.sock"));
    QCOMPARE(arguments, QStringList({ "qview", "image.jpg" }));
}

void ActionManagerTests::testIpcCurrentFilePathResponse()
{
    const QJsonObject request = { { QStringLiteral("method"), QStringLiteral("currentFilePath") } };

    QCOMPARE(QVIPCServer::handleMessage(request, "/tmp/image.jpg").value("ok").toBool(), true);
    QCOMPARE(QVIPCServer::handleMessage(request, "/tmp/image.jpg").value("path").toString(),
             QString("/tmp/image.jpg"));

    const QJsonObject noFileResponse = QVIPCServer::handleMessage(request, QString());
    QCOMPARE(noFileResponse.value("ok").toBool(), false);
    QCOMPARE(noFileResponse.value("error").toString(), QString("no_current_file"));

    const QJsonObject unknownResponse =
            QVIPCServer::handleMessage({ { QStringLiteral("method"), QStringLiteral("missing") } },
                                        "/tmp/image.jpg");
    QCOMPARE(unknownResponse.value("ok").toBool(), false);
    QCOMPARE(unknownResponse.value("error").toString(), QString("unknown_method"));
}

int main(int argc, char *argv[])
{
    QVApplication app(argc, argv);
    ActionManagerTests actionManagerTests;
    return QTest::qExec(&actionManagerTests, argc, argv);
}

#include "tst_actionmanagertests.moc"
