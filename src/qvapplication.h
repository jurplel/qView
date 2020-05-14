#ifndef QVAPPLICATION_H
#define QVAPPLICATION_H

#include "mainwindow.h"
#include "settingsmanager.h"
#include "shortcutmanager.h"
#include "actionmanager.h"

#include <QApplication>

#if defined(qvApp)
#undef qvApp
#endif

#define qvApp (qobject_cast<QVApplication *>(QCoreApplication::instance()))	// global qvapplication object

class QVApplication : public QApplication
{
    Q_OBJECT

public:
    explicit QVApplication(int &argc, char **argv);
    ~QVApplication() override;

    bool event(QEvent *event) override;

    static void openFile(MainWindow *window, const QString &file, bool resize = true);

    static void openFile(const QString &file, bool resize = true);

    static void pickFile(MainWindow *parent = nullptr);

    static void pickUrl(MainWindow *parnet = nullptr);

    static MainWindow *newWindow();

    MainWindow *getMainWindow(bool shouldBeEmpty);

    void updateDockRecents();

    qint64 getPreviouslyRecordedFileSize(const QString &fileName);

    void setPreviouslyRecordedFileSize(const QString &fileName, long long *fileSize);

    void addToLastActiveWindows(MainWindow *window);

    void deleteFromLastActiveWindows(MainWindow *window);

    void openOptionsDialog(QWidget *parent = nullptr);

    void openWelcomeDialog();

    void openAboutDialog();

    QMenuBar *getMenuBar() const {  return menuBar; }

    const QStringList &getFilterList() const { return filterList; }

    const QStringList &getNameFilterList() const { return nameFilterList; }

    SettingsManager &getSettingsManager() { return settingsManager; }

    ShortcutManager &getShortcutManager() { return shortcutManager; }

    ActionManager &getActionManager() { return actionManager; }

private:

    QList<MainWindow*> lastActiveWindows;

    QMenu *dockMenu;

    QList<QAction*> dockMenuSuffix;

    QMenu *dockMenuRecentsLibrary;

    QMenuBar *menuBar;

    QCache<QString, qint64> previouslyRecordedFileSizes;

    QStringList filterList;
    QStringList nameFilterList;

    // This order is important
    SettingsManager settingsManager; 
    ActionManager actionManager;
    ShortcutManager shortcutManager;

    QDialog *optionsDialog;
    QDialog *welcomeDialog;
    QDialog *aboutDialog;
};

#endif // QVAPPLICATION_H
