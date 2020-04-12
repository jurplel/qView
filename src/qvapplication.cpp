#include "qvapplication.h"
#include "qvoptionsdialog.h"
#include "qvoptionsdialog.h"
#include "qvaboutdialog.h"
#include "qvwelcomedialog.h"

#include <QFileOpenEvent>
#include <QSettings>
#include <QTimer>
#include <QFileDialog>

QVApplication::QVApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    // Connections
    connect(&actionManager, &ActionManager::recentsMenuUpdated, this, &QVApplication::updateDockRecents);

    // Show welcome dialog on first launch
    QSettings settings;

    if (!settings.value("firstlaunch", false).toBool())
    {
        settings.setValue("firstlaunch", true);
        settings.setValue("configversion", VERSION);
        QTimer::singleShot(100, this, &QVApplication::openWelcomeDialog);
    }

    // Initialize list of supported files and filters
    const auto byteArrayList = QImageReader::supportedImageFormats();
    for (const auto &byteArray : byteArrayList)
    {
        auto fileExtString = QString::fromUtf8(byteArray);
        filterList << "*." + fileExtString;
    }

    auto filterString = tr("Supported Files") + " (";
    for (const auto &filter : qAsConst(filterList))
    {
        filterString += filter + " ";
    }
    filterString.chop(1);
    filterString += ")";

    nameFilterList << filterString;
    nameFilterList << tr("All Files") + " (*)";

    // Don't even try to show menu icons on mac or windows
    #if defined Q_OS_MACOS || defined Q_OS_WIN
    setAttribute(Qt::AA_DontShowIconsInMenus);
    #endif

    dockMenu = new QMenu();
    connect(dockMenu, &QMenu::triggered, [](QAction *triggeredAction){
       ActionManager::actionTriggered(triggeredAction);
    });

    dockMenuSuffix.append(actionManager.cloneAction("newwindow"));
    dockMenuSuffix.append(actionManager.cloneAction("open"));

    dockMenuRecentsLibrary = nullptr;
    dockMenuRecentsLibrary = actionManager.buildRecentsMenu(false);
    actionManager.updateRecentsMenu();

    #ifdef Q_OS_MACOS
    dockMenu->setAsDockMenu();
    setQuitOnLastWindowClosed(false);
    #endif

    menuBar = actionManager.buildMenuBar();
    connect(menuBar, &QMenuBar::triggered, [](QAction *triggeredAction){
        ActionManager::actionTriggered(triggeredAction);
    });
}

QVApplication::~QVApplication() {
    dockMenu->deleteLater();
}

bool QVApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        auto *openEvent = dynamic_cast<QFileOpenEvent *>(event);
        openFile(getMainWindow(true), openEvent->file());
    }
    return QApplication::event(event);
}


void QVApplication::openFile(MainWindow *window, const QString &file, bool resize)
{
    window->setJustLaunchedWithImage(resize);
    window->openFile(file);
}

void QVApplication::pickFile(MainWindow *parent)
{
    QSettings settings;
    settings.beginGroup("recents");

    auto *fileDialog = new QFileDialog(parent, tr("Open..."));
    fileDialog->setDirectory(settings.value("lastFileDialogDir", QDir::homePath()).toString());
    fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog->setNameFilters(qvApp->getNameFilterList());

    connect(fileDialog, &QFileDialog::filesSelected, [parent](const QStringList &selected){
        bool isFirstLoop = true;
        for (const auto &file : selected)
        {
            if (isFirstLoop)
            {
                parent->openFile(file);
                isFirstLoop = false;
            }
            else
            {
                 QVApplication::openFile(QVApplication::newWindow(), file);
            }
        }

        // Set lastFileDialogDir
        QSettings settings;
        settings.beginGroup("recents");
        settings.setValue("lastFileDialogDir", QFileInfo(selected.constFirst()).path());

    });
    fileDialog->open();
}

MainWindow *QVApplication::newWindow()
{
    auto *w = new MainWindow();
    w->show();

    return w;
}

MainWindow *QVApplication::getMainWindow(bool shouldBeEmpty)
{
    // Attempt to use from list of last active windows
    for (const auto &window : qAsConst(lastActiveWindows))
    {
        if (!window)
            continue;

        if (shouldBeEmpty)
        {
            if (!window->getIsPixmapLoaded())
            {
                return window;
            }
        }
        else
        {
            return window;
        }
    }

    // If none of those are valid, scan the list for any existing MainWindow
    const auto topLevelWidgets = QApplication::topLevelWidgets();
    for (const auto &widget : topLevelWidgets)
    {
        if (auto *window = qobject_cast<MainWindow*>(widget))
        {
            if (shouldBeEmpty)
            {
                if (!window->getIsPixmapLoaded())
                {
                    return window;
                }
            }
            else
            {
                return window;
            }
        }
    }

    // If there are no valid ones, make a new one.
    auto *window = newWindow();

    return window;
}

void QVApplication::updateDockRecents()
{
    // This entire function is only necessary because invisible actions do not
    // disappear in mac's dock menu
    if (!dockMenuRecentsLibrary)
        return;

    dockMenu->clear();

    const auto dockMenuActions = dockMenuRecentsLibrary->actions();
    for (const auto &action : dockMenuActions)
    {
        if (action->isVisible())
            dockMenu->addAction(action);
    }
    if (!dockMenu->isEmpty())
        dockMenu->addSeparator();

    dockMenu->addActions(dockMenuSuffix);
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

void QVApplication::addToLastActiveWindows(MainWindow *window)
{
    if (!window)
        return;

    if (!lastActiveWindows.isEmpty() && window == lastActiveWindows.first())
        return;

    lastActiveWindows.prepend(window);

    if (lastActiveWindows.length() > 5)
        lastActiveWindows.removeLast();
}

void QVApplication::deleteFromLastActiveWindows(MainWindow *window)
{
    if (!window)
        return;

    lastActiveWindows.removeAll(window);
}

void QVApplication::openOptionsDialog()
{
    auto *options = new QVOptionsDialog();
    options->open();
}

void QVApplication::openWelcomeDialog()
{
    auto *welcome = new QVWelcomeDialog();
    welcome->open();
}

void QVApplication::openAboutDialog()
{
    auto *about = new QVAboutDialog();
    about->open();
}
