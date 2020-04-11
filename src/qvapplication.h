#ifndef QVAPPLICATION_H
#define QVAPPLICATION_H

#include <QApplication>
#include "mainwindow.h"
#include <QCache>
#include <QAction>
#include "actionmanager.h"

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

    static MainWindow *newWindow();

    MainWindow *getMainWindow(bool shouldBeEmpty);

    void updateDockRecents();

    qint64 getPreviouslyRecordedFileSize(const QString &fileName);

    void setPreviouslyRecordedFileSize(const QString &fileName, long long *fileSize);

    void addToLastActiveWindows(MainWindow *window);

    void deleteFromLastActiveWindows(MainWindow *window);

    ActionManager *getActionManager() const { return actionManager; }

    QMenuBar *getMenuBar() const {  return menuBar; }

    const QStringList &getFilterList() const { return filterList; }

    const QStringList &getNameFilterList() const { return nameFilterList; }

private:
    QStringList filterList;
    QStringList nameFilterList;

    QList<MainWindow*> lastActiveWindows;

    QMenu *dockMenu;

    QList<QAction*> dockMenuPrefix;

    QMenu *dockMenuRecentsLibrary;

    QMenuBar *menuBar;

    QCache<QString, qint64> previouslyRecordedFileSizes;

    ActionManager *actionManager;

};

#endif // QVAPPLICATION_H
