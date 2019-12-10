#include "qvapplication.h"
#include "qvoptionsdialog.h"
#include <QFileOpenEvent>
#include <QMenu>
#include <QSettings>
#include <QMenuBar>

#include <QDebug>

QVApplication::QVApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    //don't even try to show menu icons on mac or windows
    #if defined(Q_OS_MACX) || defined(Q_OS_WIN)
    setAttribute(Qt::AA_DontShowIconsInMenus);
    #endif

    dockMenu = new QMenu();
    #ifdef Q_OS_MACX
    dockMenu->setAsDockMenu();
    setQuitOnLastWindowClosed(false);
    #endif
    updateDockRecents();

    QMenuBar *globalMenuBar = new QMenuBar();
    auto fileMenu = new QMenu("File");
    globalMenuBar->addMenu(fileMenu);

    newWindowAction = new QAction("New Window");
    connect(newWindowAction, &QAction::triggered, [](){
        newWindow();
    });
    fileMenu->addAction(newWindowAction);

    openAction = new QAction("Open");
    connect(openAction, &QAction::triggered, []() {
       pickFile();
    });
    fileMenu->addAction(openAction);

    loadShortcuts();
}

QVApplication::~QVApplication() {
    dockMenu->deleteLater();
}

bool QVApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        auto *openEvent = dynamic_cast<QFileOpenEvent *>(event);
        openFile(openEvent->file());
    }
    return QApplication::event(event);
}

void QVApplication::pickFile()
{
    MainWindow *w = getMainWindow();

    if (!w)
        return;

    w->pickFile();
}

void QVApplication::openFile(const QString &file, bool resize)
{
    MainWindow *w = getMainWindow();

    if (!w)
        return;

    w->setJustLaunchedWithImage(resize);
    w->openFile(file);
}

MainWindow *QVApplication::newWindow(const QString &fileToOpen)
{
    auto *w = new MainWindow();
    w->show();

    if (!fileToOpen.isEmpty())
        w->openFile(fileToOpen);

    return w;
}

MainWindow *QVApplication::getMainWindow()
{
    auto widget = activeWindow();

    if (widget)
    {
        while (widget->parentWidget() != nullptr)
            widget = widget->parentWidget();
        qDebug() << 0;

        return qobject_cast<MainWindow*>(widget);
    }

    MainWindow *w = nullptr;

    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        if (auto *mainWin = qobject_cast<MainWindow*>(widget))
        {
            if (!mainWin->getIsPixmapLoaded())
            {
                w = mainWin;
            }
        }
    }

    if (!w)
        w = newWindow();

    return w;
}

void QVApplication::updateDockRecents()
{
    QSettings settings;
    settings.beginGroup("options");

    bool isSaveRecentsEnabled = settings.value("saverecents", true).toBool();

    settings.endGroup();

    dockMenu->clear();
    if (isSaveRecentsEnabled) {
        settings.beginGroup("recents");
        QVariantList recentFiles = settings.value("recentFiles").value<QVariantList>();

        for (int i = 0; i <= 9; i++)
        {
            if (i < recentFiles.size())
            {
                auto *action = new QAction(recentFiles[i].toList().first().toString());
                connect(action, &QAction::triggered, [recentFiles, i]{
                   openFile(recentFiles[i].toList().last().toString(), false);
                });
                dockMenu->addAction(action);
            }
        }
        dockMenu->addSeparator();
    }
    auto *newWindowAction = new QAction(tr("New Window"));
    connect(newWindowAction, &QAction::triggered, []{
        newWindow();
    });
    auto *openAction = new QAction(tr("Open..."));
    connect(openAction, &QAction::triggered, []{
        pickFile();
    });
    dockMenu->addAction(newWindowAction);
    dockMenu->addAction(openAction);
}

qint64 QVApplication::getPreviouslyRecordedFileSize(const QString &fileName)
{
    auto previouslyRecordedFileSizePtr = previouslyRecordedFileSizes.object(fileName);
    qint64 previouslyRecordedFileSize = 0;

    if (previouslyRecordedFileSizePtr)
        previouslyRecordedFileSize = *previouslyRecordedFileSizePtr;

    return previouslyRecordedFileSize;
}

void QVApplication::setPreviouslyRecordedFileSize(const QString &fileName, long long *fileSize)
{
    previouslyRecordedFileSizes.insert(fileName, fileSize);
}

void QVApplication::loadShortcuts()
{
    auto shortcuts = getShortcutsList();
    newWindowAction->setShortcuts(shortcuts.value("newwindow"));
    openAction->setShortcuts(shortcuts.value("open"));
}

QHash<QString, QList<QKeySequence>> QVApplication::getShortcutsList()
{
    QSettings settings;
    settings.beginGroup("shortcuts");

    // To retrieve default bindings, we hackily init an options dialog and use it's constructor values
    QVOptionsDialog invisibleOptionsDialog;
    auto shortcutData = invisibleOptionsDialog.getTransientShortcuts();

    // Iterate through all default shortcuts to get saved shortcuts from settings
    QHash<QString, QList<QKeySequence>> shortcuts;
    QListIterator<QVShortcutDialog::SShortcut> iter(shortcutData);
    while (iter.hasNext())
    {
        auto value = iter.next();
        shortcuts.insert(value.name, QVOptionsDialog::stringListToKeySequenceList(settings.value(value.name, value.defaultShortcuts).value<QStringList>()));
    }
    return shortcuts;
}
