#include "qvapplication.h"
#include "qvoptionsdialog.h"
#include <QFileOpenEvent>
#include <QMenu>
#include <QSettings>
#include <QMenuBar>

#include <QDebug>

QVApplication::QVApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    actionManager = new ActionManager(this);

    //don't even try to show menu icons on mac or windows
    #if defined(Q_OS_MACX) || defined(Q_OS_WIN)
    setAttribute(Qt::AA_DontShowIconsInMenus);
    #endif

    dockMenu = new QMenu();
    #ifdef Q_OS_MACX
    dockMenu->setAsDockMenu();
    setQuitOnLastWindowClosed(false);
    actionManager->buildMenuBar();
    #endif

    updateDockRecents();
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
    MainWindow *w = getEmptyMainWindow();

    if (!w)
        return;

    w->pickFile();
}

void QVApplication::openFile(const QString &file, bool resize)
{
    MainWindow *w = getEmptyMainWindow();

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

MainWindow *QVApplication::getCurrentMainWindow()
{
    auto activeWidget = activeWindow();

    if (activeWidget)
    {
        while (activeWidget->parentWidget() != nullptr)
            activeWidget = activeWidget->parentWidget();

        if (auto *window = qobject_cast<MainWindow*>(activeWidget))
        {
            return window;
        }
    }

    return nullptr;
}

MainWindow *QVApplication::getEmptyMainWindow()
{
    //Attempt to use the active window
    if (auto *window = getCurrentMainWindow())
    {
        if (!window->getIsPixmapLoaded())
        {
            return window;
        }
    }

    //If that is not valid, check all windows and use the first valid one
    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        if (auto *window = qobject_cast<MainWindow*>(widget))
        {
            if (!window->getIsPixmapLoaded())
            {
                return window;
            }
        }
    }

    //If there are no valid ones, make a new one.
    auto *window = newWindow();

    return window;
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
