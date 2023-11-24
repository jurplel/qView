#include "qvapplication.h"
#include "qvoptionsdialog.h"
#include "qvcocoafunctions.h"
#include "updatechecker.h"

#include <QFileOpenEvent>
#include <QSettings>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QFontDatabase>

QVApplication::QVApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    setDesktopFileName("com.interversehq.qView.desktop");

    // Connections
    connect(this, &QGuiApplication::commitDataRequest, this, &QVApplication::onCommitDataRequest, Qt::DirectConnection);
    connect(this, &QCoreApplication::aboutToQuit, this, &QVApplication::onAboutToQuit);
    connect(&settingsManager, &SettingsManager::settingsUpdated, this, &QVApplication::settingsUpdated);
    connect(&actionManager, &ActionManager::recentsMenuUpdated, this, &QVApplication::recentsMenuUpdated);
    connect(&updateChecker, &UpdateChecker::checkedUpdates, this, &QVApplication::checkedUpdates);

    // Add fallback fromTheme icon search on linux with qt >5.11
#if defined Q_OS_UNIX && !defined Q_OS_MACOS && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    QIcon::setFallbackSearchPaths(QIcon::fallbackSearchPaths() << "/usr/share/pixmaps");
#endif

    settingsUpdated();

    // Check for updates
    // TODO: move this to after first window show event
    if (getSettingsManager().getBoolean("updatenotifications"))
        updateChecker.check();

    showSubmenuIcons = getSettingsManager().getBoolean("submenuicons");

    // Setup macOS dock menu
    dockMenu = new QMenu();
    connect(dockMenu, &QMenu::triggered, this, [](QAction *triggeredAction){
       ActionManager::actionTriggered(triggeredAction);
    });

    actionManager.loadRecentsList();

#ifdef Q_OS_MACOS
    actionManager.addCloneOfAction(dockMenu, "newwindow");
    actionManager.addCloneOfAction(dockMenu, "open");
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

    // Block any erroneous icons from showing up on mac and windows
    // (this is overridden in some cases)
#if defined Q_OS_MACOS || defined Q_OS_WIN
    setAttribute(Qt::AA_DontShowIconsInMenus);
#endif
    // Adwaita Qt styles should hide icons for a more consistent look
    if (style()->objectName() == "adwaita-dark" || style()->objectName() == "adwaita")
        setAttribute(Qt::AA_DontShowIconsInMenus);

    hideIncompatibleActions();
}

QVApplication::~QVApplication()
{
    dockMenu->deleteLater();
    menuBar->deleteLater();
}

bool QVApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen)
    {
        auto *openEvent = static_cast<QFileOpenEvent *>(event);
        openFile(getMainWindow(true), openEvent->file());
    }
    else if (event->type() == QEvent::ApplicationStateChange)
    {
        auto *stateEvent = static_cast<QApplicationStateChangeEvent*>(event);
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

MainWindow *QVApplication::newWindow(const QJsonObject &windowSessionState)
{
    auto *w = new MainWindow(nullptr, windowSessionState);
    w->show();
    w->raise();

    return w;
}

MainWindow *QVApplication::getMainWindow(bool shouldBeEmpty)
{
    MainWindow *foundWindow = nullptr;

    for (MainWindow *window : qAsConst(activeWindows))
    {
        // If an empty window is requested, check this flag because it gets set right
        // after a load is requested, so it will be set if an image is already loaded
        // or if a load is currently in progress
        if (shouldBeEmpty && window->getCurrentFileDetails().isLoadRequested)
            continue;

        if (foundWindow && foundWindow->getLastActivatedTimestamp() >= window->getLastActivatedTimestamp())
            continue;

        foundWindow = window;
    }

    return foundWindow ? foundWindow : newWindow();
}

void QVApplication::checkedUpdates()
{
    const UpdateChecker::CheckResult checkResult = updateChecker.getCheckResult();

    QWidget *dialogParent = aboutDialog ? aboutDialog : nullptr;

    if (checkResult.wasSuccessful && checkResult.isConsideredUpdate())
    {
        updateChecker.openDialog(dialogParent, !aboutDialog);
    }
    else if (aboutDialog)
    {
        if (!checkResult.wasSuccessful)
            QMessageBox::critical(dialogParent, tr("Error"), tr("Error checking for updates:\n%1").arg(checkResult.errorMessage));
        else
            QMessageBox::information(dialogParent, tr("No Updates"), tr("You already have the latest version."));
    }

    if (aboutDialog)
        aboutDialog->updateCheckForUpdatesButtonState();
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

void QVApplication::addToActiveWindows(MainWindow *window)
{
    if (!window)
        return;

    activeWindows.insert(window);
}

void QVApplication::deleteFromActiveWindows(MainWindow *window)
{
    if (!window)
        return;

    activeWindows.remove(window);
}

bool QVApplication::foundLoadedImage() const
{
    for (MainWindow *window : qAsConst(activeWindows))
    {
        if (window->getIsPixmapLoaded())
            return true;
    }
    return false;
}

void QVApplication::openOptionsDialog(QWidget *parent)
{
#ifdef Q_OS_MACOS
    // On macOS, the dialog should not be dependent on any window
    parent = nullptr;
#endif


    if (optionsDialog)
    {
        optionsDialog->raise();
        optionsDialog->activateWindow();
        return;
    }

    optionsDialog = new QVOptionsDialog(parent);
    optionsDialog->show();
}

void QVApplication::openWelcomeDialog(QWidget *parent)
{
#ifdef Q_OS_MACOS
    // On macOS, the dialog should not be dependent on any window
    parent = nullptr;
#endif

    if (welcomeDialog)
    {
        welcomeDialog->raise();
        welcomeDialog->activateWindow();
        return;
    }

    welcomeDialog = new QVWelcomeDialog(parent);
    welcomeDialog->show();
}

void QVApplication::openAboutDialog(QWidget *parent)
{
#ifdef Q_OS_MACOS
    // On macOS, the dialog should not be dependent on any window
    parent = nullptr;
#endif

    if (aboutDialog)
    {
        aboutDialog->raise();
        aboutDialog->activateWindow();
        return;
    }

    aboutDialog = new QVAboutDialog(parent);
    aboutDialog->show();
}

void QVApplication::hideIncompatibleActions()
{    
    // Deletion actions
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    auto hideDeleteActions = [this]{
        getActionManager().hideAllInstancesOfAction("delete");
        getActionManager().hideAllInstancesOfAction("undo");

        getShortcutManager().setShortcutsHidden({"delete", "undo"});
    };
#if defined Q_OS_UNIX && !defined Q_OS_MACOS
    QProcess *testGio = new QProcess(this);
    testGio->start("gio", QStringList());
    connect(testGio, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [hideDeleteActions, testGio, this](){
        if (testGio->error() == QProcess::FailedToStart)
        {
            qInfo() << "No backup gio trash backend found";
            hideDeleteActions();
        }
        else
        {
            qInfo() << "Using backup gio trash backend";
        }
    });
#elif defined Q_OS_WIN || (defined Q_OS_MACOS && !COCOA_LOADED)
    qInfo() << "Qt version too old for trash feature";
    hideDeleteActions();
#endif
#endif
}

void QVApplication::settingsUpdated()
{
    auto &settingsManager = qvApp->getSettingsManager();

    QString disabledFileExtensionsStr = settingsManager.getString("disabledfileextensions");
    disabledFileExtensions = Qv::listToSet(!disabledFileExtensionsStr.isEmpty() ? disabledFileExtensionsStr.split(';') : QStringList());

#ifdef Q_OS_MACOS
    setQuitOnLastWindowClosed(settingsManager.getBoolean("quitonlastwindow"));
#endif

    defineFilterLists();
}

void QVApplication::defineFilterLists()
{
    allFileExtensionList.clear();
    nameFilterList.clear();
    fileExtensionSet.clear();
    mimeTypeNameSet.clear();

    const auto &byteArrayFormats = QImageReader::supportedImageFormats();

    auto filterString = tr("Supported Images") + " (";
    fileExtensionSet.reserve(byteArrayFormats.size()-1);

    const auto addExtension = [&](const QString &extension) {
        allFileExtensionList << extension;
        if (disabledFileExtensions.contains(extension))
            return;
        filterString += "*" + extension + " ";
        fileExtensionSet << extension;
    };

    // Build extension and filter lists
    for (const auto &byteArray : byteArrayFormats)
    {
        const auto fileExtension = "." + QString::fromUtf8(byteArray);
        // Qt 5.15 seems to have added pdf support for QImageReader but it is super broken in qView
        if (fileExtension == ".pdf")
            continue;

        addExtension(fileExtension);

        // If we support jpg, we actually support the jfif, jfi, and jpe file extensions too almost certainly.
        if (fileExtension == ".jpg")
        {
            addExtension(".jpe");
            addExtension(".jfi");
            addExtension(".jfif");
        }
    }
    filterString.chop(1);
    filterString += ")";

    // Build mime type list
    const auto &byteArrayMimeTypes = QImageReader::supportedMimeTypes();
    mimeTypeNameSet.reserve(byteArrayMimeTypes.size()-1);
    for (const auto &byteArray : byteArrayMimeTypes)
    {
        // Qt 5.15 seems to have added pdf support for QImageReader but it is super broken in qView
        const QString mime = QString::fromUtf8(byteArray);
        if (mime == "application/pdf")
            continue;

        mimeTypeNameSet << mime;
    }

    // Build name filter list for file dialogs
    nameFilterList << filterString;
    nameFilterList << tr("All Files") + " (*)";
}

void QVApplication::ensureFontLoaded(const QString &path)
{
    if (loadedFontPaths.contains(path))
        return;

    QFontDatabase::addApplicationFont(path);
    loadedFontPaths.insert(path);
}

QIcon QVApplication::iconFromFont(const QString &fontFamily, const QChar &codePoint, const int pixelSize, const qreal pixelRatio)
{
    const int scaledPixelSize = qRound(pixelSize * pixelRatio);

    QFont font(fontFamily);
    font.setPixelSize(pixelSize);

    QPixmap pixmap(scaledPixelSize, scaledPixelSize);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(pixelRatio);

    QPainter painter(&pixmap);
    painter.setFont(font);
    painter.setPen(QApplication::palette().color(QPalette::WindowText));
    painter.drawText(pixmap.rect(), codePoint);

    return QIcon(pixmap);
}

bool QVApplication::supportsSessionPersistence()
{
#if defined(Q_OS_MACOS)
    return true;
#else
    return false;
#endif
}

bool QVApplication::tryRestoreLastSession()
{
    if (!supportsSessionPersistence())
        return false;

    QSettings settings;

    if (!settings.value("options/persistsession").toBool())
        return false;

    const QJsonObject sessionState = settings.value("sessionstate").toJsonObject();

    if (sessionState.isEmpty() || sessionState["version"].toInt() != Qv::SessionStateVersion)
        return false;

    const QJsonArray windowArray = sessionState["windows"].toArray();
    for (const QJsonValue &item : windowArray)
    {
        QVApplication::newWindow(item.toObject());
    }

    settings.remove("sessionstate");

    return true;
}

void QVApplication::legacyQuit()
{
    isApplicationQuitting = true;
    qGuiApp->postEvent(qGuiApp, new QEvent(QEvent::Quit));
}

bool QVApplication::getIsApplicationQuitting() const
{
    return isApplicationQuitting;
}

bool QVApplication::isSessionStateEnabled() const
{
    return supportsSessionPersistence() && getSettingsManager().getBoolean("persistsession");
}

void QVApplication::setUserDeclinedSessionStateSave(const bool value)
{
    userDeclinedSessionStateSave = value;
}

bool QVApplication::isSessionStateSaveRequested() const
{
    return getIsApplicationQuitting() && isSessionStateEnabled() && !userDeclinedSessionStateSave;
}

void QVApplication::addClosedWindowSessionState(const QJsonObject &state, const qint64 lastActivatedTimestamp)
{
    closedWindowData.append({state, lastActivatedTimestamp});
}

void QVApplication::onCommitDataRequest(QSessionManager &manager)
{
    Q_UNUSED(manager)

    isApplicationQuitting = true;
}

void QVApplication::onAboutToQuit()
{
    if (isSessionStateSaveRequested())
    {
        QSettings settings;
        if (!closedWindowData.isEmpty())
        {
            QJsonObject state;

            state["version"] = Qv::SessionStateVersion;

            std::sort(
                closedWindowData.begin(), closedWindowData.end(),
                [](const ClosedWindowData& a, const ClosedWindowData& b) {
                    return a.lastActivatedTimestamp < b.lastActivatedTimestamp;
                });
            QJsonArray windows;
            for (const ClosedWindowData& item : closedWindowData)
                windows.append(item.sessionState);
            state["windows"] = windows;

            settings.setValue("sessionstate", state);
        }
        else
        {
            settings.remove("sessionstate");
        }
    }
}
