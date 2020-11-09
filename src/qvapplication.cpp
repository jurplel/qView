#include "qvapplication.h"
#include "qvoptionsdialog.h"
#include "qvcocoafunctions.h"
#include "updatechecker.h"

#include <QFileOpenEvent>
#include <QSettings>
#include <QTimer>
#include <QFileDialog>

QVApplication::QVApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    // Connections
    connect(&actionManager, &ActionManager::recentsMenuUpdated, this, &QVApplication::recentsMenuUpdated);
    connect(&updateChecker, &UpdateChecker::checkedUpdates, this, &QVApplication::checkedUpdates);

    // Initialize variables
    optionsDialog = nullptr;
    welcomeDialog = nullptr;
    aboutDialog = nullptr;

    // Initialize list of supported files and filters
    const auto byteArrayList = QImageReader::supportedImageFormats();
    for (const auto &byteArray : byteArrayList)
    {
        auto fileExtString = QString::fromUtf8(byteArray);
        // Qt 5.15 seems to have added pdf support for QImageReader but it is super broken in qView at the moment
        if (fileExtString == "pdf")
            continue;

        filterList << "*." + fileExtString;
    }

    auto filterString = tr("Supported Images") + " (";
    for (const auto &filter : qAsConst(filterList))
    {
        filterString += filter + " ";
    }
    filterString.chop(1);
    filterString += ")";

    nameFilterList << filterString;
    nameFilterList << tr("All Files") + " (*)";

    // Check for updates
    // TODO: move this to after first window show event
    if (getSettingsManager().getBoolean("updatenotifications"))
        checkUpdates();

    // Setup macOS dock menu
    dockMenu = new QMenu();
    connect(dockMenu, &QMenu::triggered, this, [](QAction *triggeredAction){
       ActionManager::actionTriggered(triggeredAction);
    });

    actionManager.loadRecentsList();

#ifdef Q_OS_MACOS
    dockMenu->addAction(actionManager.cloneAction("newwindow"));
    dockMenu->addAction(actionManager.cloneAction("open"));
    dockMenu->setAsDockMenu();
#endif

    // Build menu bar
    menuBar = actionManager.buildMenuBar();
    connect(menuBar, &QMenuBar::triggered, this, [](QAction *triggeredAction){
        ActionManager::actionTriggered(triggeredAction);
    });

    // Set mac-specific application settings
#ifdef COCOA_LOADED
    QVCocoaFunctions::setUserDefaults();
#endif
#ifdef Q_OS_MACOS
    setQuitOnLastWindowClosed(false);
#endif

    // Block any erroneous icons from showing up on mac and windows
    // (this is overridden in some cases)
#if defined Q_OS_MACOS || defined Q_OS_WIN
    setAttribute(Qt::AA_DontShowIconsInMenus);
#endif
    // Adwaita Qt styles should hide icons for a more consistent look
    if (style()->objectName() == "adwaita-dark" || style()->objectName() == "adwaita")
        setAttribute(Qt::AA_DontShowIconsInMenus);
}

QVApplication::~QVApplication() {
    dockMenu->deleteLater();
}

bool QVApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen)
    {
        auto *openEvent = dynamic_cast<QFileOpenEvent *>(event);
        openFile(getMainWindow(true), openEvent->file());
    }
    else if (event->type() == QEvent::ApplicationStateChange)
    {
        auto *stateEvent = dynamic_cast<QApplicationStateChangeEvent*>(event);
        if (stateEvent->applicationState() == Qt::ApplicationActive)
            settingsManager.loadSettings();
    }
    return QApplication::event(event);
}

void QVApplication::openFile(MainWindow *window, const QString &file, bool resize)
{
    window->setJustLaunchedWithImage(resize);
    window->openFile(file);
}

void QVApplication::openFile(const QString &file, bool resize)
{
    auto *window = qvApp->getMainWindow(true);

    QVApplication::openFile(window, file, resize);
}

void QVApplication::pickFile(MainWindow *parent)
{
    QSettings settings;
    settings.beginGroup("recents");

    auto *fileDialog = new QFileDialog(parent, tr("Open..."));
    fileDialog->setDirectory(settings.value("lastFileDialogDir", QDir::homePath()).toString());
    fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog->setNameFilters(qvApp->getNameFilterList());
    if (parent)
        fileDialog->setWindowModality(Qt::WindowModal);

    connect(fileDialog, &QFileDialog::filesSelected, fileDialog, [parent](const QStringList &selected){
        bool isFirstLoop = true;
        for (const auto &file : selected)
        {
            if (isFirstLoop && parent)
                parent->openFile(file);
            else
                QVApplication::openFile(file);

            isFirstLoop = false;
        }

        // Set lastFileDialogDir
        QSettings settings;
        settings.beginGroup("recents");
        settings.setValue("lastFileDialogDir", QFileInfo(selected.constFirst()).path());

    });
    fileDialog->show();
}

MainWindow *QVApplication::newWindow()
{
    auto *w = new MainWindow();
    w->show();
    w->raise();

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
            // File info is set if an image load is requested, but not loaded
            if (!window->getCurrentFileDetails().isLoadRequested)
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
                if (!window->getCurrentFileDetails().isLoadRequested)
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

void QVApplication::checkUpdates()
{
    updateChecker.check();
}

void QVApplication::checkedUpdates()
{
    if (aboutDialog)
    {
        aboutDialog->setLatestVersionNum(updateChecker.getLatestVersionNum());
    }
    else if (updateChecker.getLatestVersionNum() > VERSION &&
             getSettingsManager().getBoolean("updatenotifications"))
    {
        updateChecker.openDialog();
    }
}

void QVApplication::recentsMenuUpdated()
{
#ifdef COCOA_LOADED
    QStringList recentsPathList;
    for(const auto &recent : actionManager.getRecentsList())
    {
        recentsPathList << recent.filePath;
    }
    QVCocoaFunctions::setDockRecents(recentsPathList);
#endif
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

void QVApplication::openOptionsDialog(QWidget *parent)
{
    // On macOS, the dialog should not be dependent on any window
    if (optionsDialog)
    {
        optionsDialog->raise();
        optionsDialog->activateWindow();
        return;

    }

    optionsDialog = new QVOptionsDialog(parent);
    connect(optionsDialog, &QDialog::finished, this, [this]{
        optionsDialog = nullptr;
    });
    optionsDialog->show();
}

void QVApplication::openWelcomeDialog(QWidget *parent)
{
    if (welcomeDialog)
    {
        welcomeDialog->raise();
        welcomeDialog->activateWindow();
        return;
    }

    welcomeDialog = new QVWelcomeDialog(parent);
    connect(welcomeDialog, &QDialog::finished, this, [this]{
        welcomeDialog = nullptr;
    });
    welcomeDialog->show();
}

void QVApplication::openAboutDialog(QWidget *parent)
{
    if (aboutDialog)
    {
        aboutDialog->raise();
        aboutDialog->activateWindow();
        return;
    }

    aboutDialog = new QVAboutDialog(updateChecker.getLatestVersionNum(), parent);
    connect(aboutDialog, &QDialog::finished, this, [this]{
        aboutDialog = nullptr;
    });
    aboutDialog->show();
}
