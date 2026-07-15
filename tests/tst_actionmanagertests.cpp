#include <QtTest>
#include <QSettings>
#include <QStandardPaths>

#include "qvapplication.h"

class TestMainWindow : public MainWindow
{
public:
    TestMainWindow() { setAttribute(Qt::WA_DeleteOnClose, false); }
    ~TestMainWindow() override { close(); }

    bool triggerEscapeShortcut()
    {
        auto *shortcut = findChild<QShortcut *>("escapeShortcut");
        return shortcut
                && QMetaObject::invokeMethod(shortcut, "activated", Qt::DirectConnection);
    }
    void refreshShortcuts() { shortcutsUpdated(); }
    int exitRequestCount() const { return requestedExitCount; }

protected:
    void requestApplicationExit() override { ++requestedExitCount; }

private:
    int requestedExitCount = 0;
};

class ActionManagerTests : public QObject
{
    Q_OBJECT

public:
    ActionManagerTests();
    ~ActionManagerTests();

private slots:
    void testEscapeActionForWindowState_data();
    void testEscapeActionForWindowState();
    void testEscapeShortcutConfiguration();
    void testEscapeInNormalModeRequestsExitOnce();
    void testEscapeInFullScreenRestoresWindow();
    void testEscapeConflictDisablesDedicatedShortcut();
    void testNullActionIsIgnored();
    void testClonedActionsUntracked();
};

ActionManagerTests::ActionManagerTests() { }

ActionManagerTests::~ActionManagerTests() { }

void ActionManagerTests::testEscapeActionForWindowState_data()
{
    QTest::addColumn<int>("windowState");
    QTest::addColumn<int>("expectedAction");

    QTest::newRow("normal") << int(Qt::WindowNoState)
                            << int(MainWindow::EscapeAction::QuitApplication);
    QTest::newRow("maximized") << int(Qt::WindowMaximized)
                               << int(MainWindow::EscapeAction::QuitApplication);
    QTest::newRow("minimized") << int(Qt::WindowMinimized)
                               << int(MainWindow::EscapeAction::QuitApplication);
    QTest::newRow("fullscreen") << int(Qt::WindowFullScreen)
                                << int(MainWindow::EscapeAction::ExitFullScreen);
    QTest::newRow("fullscreen-active") << int(Qt::WindowFullScreen | Qt::WindowActive)
                                       << int(MainWindow::EscapeAction::ExitFullScreen);
}

void ActionManagerTests::testEscapeActionForWindowState()
{
    QFETCH(int, windowState);
    QFETCH(int, expectedAction);

    const auto action =
            MainWindow::escapeActionForWindowState(Qt::WindowStates(windowState));

    QCOMPARE(int(action), expectedAction);
}

void ActionManagerTests::testEscapeShortcutConfiguration()
{
    TestMainWindow window;
    auto *shortcut = window.findChild<QShortcut *>("escapeShortcut");

    QVERIFY(shortcut);
    QCOMPARE(shortcut->key(), QKeySequence(Qt::Key_Escape));
    QCOMPARE(shortcut->context(), Qt::WindowShortcut);
    QVERIFY(!shortcut->autoRepeat());
}

void ActionManagerTests::testEscapeInNormalModeRequestsExitOnce()
{
    TestMainWindow window;

    QVERIFY(window.triggerEscapeShortcut());
    QVERIFY(window.triggerEscapeShortcut());

    QCOMPARE(window.exitRequestCount(), 1);
}

void ActionManagerTests::testEscapeInFullScreenRestoresWindow()
{
    TestMainWindow window;
    window.setWindowState(Qt::WindowFullScreen);

    QVERIFY(window.triggerEscapeShortcut());

    QVERIFY(!window.windowState().testFlag(Qt::WindowFullScreen));
    QCOMPARE(window.exitRequestCount(), 0);
}

void ActionManagerTests::testEscapeConflictDisablesDedicatedShortcut()
{
    TestMainWindow window;
    auto *shortcut = window.findChild<QShortcut *>("escapeShortcut");
    auto *openAction = qvApp->getActionManager().getAction("open");
    QVERIFY(shortcut);
    QVERIFY(openAction);

    const auto originalShortcuts = openAction->shortcuts();
    openAction->setShortcut(QKeySequence(Qt::Key_Escape));
    window.refreshShortcuts();
    const bool disabledForConflict = shortcut->key().isEmpty();

    openAction->setShortcuts(originalShortcuts);
    window.refreshShortcuts();

    QVERIFY(disabledForConflict);
    QCOMPARE(shortcut->key(), QKeySequence(Qt::Key_Escape));
}

void ActionManagerTests::testNullActionIsIgnored()
{
    ActionManager::actionTriggered(nullptr);
    ActionManager::actionTriggered(nullptr, nullptr);
}

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

int main(int argc, char *argv[])
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("qViewTests");
    QCoreApplication::setApplicationName("qViewTests");
    QSettings().clear();
    QSettings().setValue("firstlaunch", true);

    QVApplication app(argc, argv);
    ActionManagerTests actionManagerTests;
    return QTest::qExec(&actionManagerTests, argc, argv);
}

#include "tst_actionmanagertests.moc"
