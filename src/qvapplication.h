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
#define qvApp (static_cast<QVApplication *>(QCoreApplication::instance()))	// global qvapplication object

class QVApplication : public QApplication
{
    Q_OBJECT

public:
    explicit QVApplication(int &argc, char **argv);
    ~QVApplication() override;

    bool event(QEvent *event) override;

    static void pickFile();

    static void openFile(const QString &file, bool resize = true);

    static MainWindow *newWindow(const QString &fileToOpen = "");

    static MainWindow *getCurrentMainWindow();

    static MainWindow *getEmptyMainWindow();

    void updateDockRecents();

    qint64 getPreviouslyRecordedFileSize(const QString &fileName);

    void setPreviouslyRecordedFileSize(const QString &fileName, long long *fileSize);

    QHash<QString, QList<QKeySequence>> getShortcutsList();

    ActionManager *getActionManager() const {return actionManager; }

private:
    QMenu *dockMenu;

    QCache<QString, qint64> previouslyRecordedFileSizes;

    ActionManager *actionManager;

};

#endif // QVAPPLICATION_H
