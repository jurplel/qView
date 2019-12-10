#ifndef QVAPPLICATION_H
#define QVAPPLICATION_H

#include <QApplication>
#include "mainwindow.h"
#include <QCache>
#include <QAction>

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

    static MainWindow *getMainWindow();

    void updateDockRecents();

    qint64 getPreviouslyRecordedFileSize(const QString &fileName);

    void setPreviouslyRecordedFileSize(const QString &fileName, long long *fileSize);

    QHash<QString, QList<QKeySequence>> getShortcutsList();

private:
    QMenu *dockMenu;

    QCache<QString, qint64> previouslyRecordedFileSizes;

};

#endif // QVAPPLICATION_H
