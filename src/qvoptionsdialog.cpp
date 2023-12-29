#include "qvoptionsdialog.h"
#include "ui_qvoptionsdialog.h"
#include "qvapplication.h"
#include <QColorDialog>
#include <QPalette>
#include <QScreen>
#include <QMessageBox>
#include <QSettings>

#include <QDebug>

QVOptionsDialog::QVOptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVOptionsDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint | Qt::CustomizeWindowHint));

    resize(640, 530);

    qvApp->ensureFontLoaded(":/fonts/MaterialIconsOutlined-Regular.otf");

    connect(ui->categoryList, &QListWidget::currentRowChanged, this, [this](int currentRow) { ui->stackedWidget->setCurrentIndex(currentRow); });
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &QVOptionsDialog::buttonBoxClicked);
    connect(ui->shortcutsTable, &QTableWidget::cellDoubleClicked, this, &QVOptionsDialog::shortcutCellDoubleClicked);
    connect(ui->bgColorCheckbox, &QCheckBox::stateChanged, this, &QVOptionsDialog::bgColorCheckboxStateChanged);
    connect(ui->nonNativeThemeCheckbox, &QCheckBox::stateChanged, this, [this](int state) { restartNotifyForCheckbox("nonnativetheme", state); });
    connect(ui->submenuIconsCheckbox, &QCheckBox::stateChanged, this, [this](int state) { restartNotifyForCheckbox("submenuicons", state); });
    connect(ui->scalingCheckbox, &QCheckBox::stateChanged, this, &QVOptionsDialog::scalingCheckboxStateChanged);
    connect(ui->constrainImagePositionCheckbox, &QCheckBox::stateChanged, this, &QVOptionsDialog::constrainImagePositionCheckboxStateChanged);
    connect(ui->middleButtonModeClickRadioButton, &QRadioButton::clicked, this, &QVOptionsDialog::middleButtonModeChanged);
    connect(ui->middleButtonModeDragRadioButton, &QRadioButton::clicked, this, &QVOptionsDialog::middleButtonModeChanged);
    connect(ui->titlebarComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QVOptionsDialog::titlebarComboBoxCurrentIndexChanged);
    connect(ui->windowResizeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QVOptionsDialog::windowResizeComboBoxCurrentIndexChanged);
    connect(ui->langComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QVOptionsDialog::languageComboBoxCurrentIndexChanged);
    connect(ui->formatsTable, &QTableWidget::itemChanged, this, &QVOptionsDialog::formatsItemChanged);

    populateCategories();
    populateComboBoxes();
    populateLanguages();

    QSettings settings;
    ui->categoryList->setCurrentRow(settings.value("optionstab", 1).toInt());

    // On macOS, the dialog should not be dependent on any window
#ifndef Q_OS_MACOS
    setWindowModality(Qt::WindowModal);
#else
    // Load window geometry
    restoreGeometry(settings.value("optionsgeometry").toByteArray());
#endif


    if (QOperatingSystemVersion::current() < QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 13)) {
        setWindowTitle(tr("Preferences"));
    }

// Platform specific settings
#if !defined(Q_OS_WIN) || QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
    ui->nonNativeThemeCheckbox->hide();
#endif
#ifdef Q_OS_MACOS
    ui->menubarCheckbox->hide();
#else
    ui->darkTitlebarCheckbox->hide();
    ui->quitOnLastWindowCheckbox->hide();
#endif

    if (!QVApplication::supportsSessionPersistence())
        ui->persistSessionCheckbox->hide();

// Hide language selection below 5.12, as 5.12 does not support embedding the translations :(
#if (QT_VERSION < QT_VERSION_CHECK(5, 12, 0))
    ui->langComboBox->hide();
    ui->langComboLabel->hide();
#endif

// Hide color space conversion below 5.14, which is when color space support was introduced
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
    ui->colorSpaceConversionComboBox->hide();
    ui->colorSpaceConversionLabel->hide();
#endif

    QString ctrlKeyName = QKeySequence(Qt::CTRL).toString(QKeySequence::NativeText).replace(QRegularExpression("\\+$"), "");
    ui->altDoubleClickLabel->setText(tr("%1 + Double Click:").arg(ctrlKeyName));
    ui->altDragLabel->setText(tr("%1 + Drag:").arg(ctrlKeyName));
    ui->altMiddleClickLabel->setText(tr("%1 + Middle Click:").arg(ctrlKeyName));
    ui->altMiddleDragLabel->setText(tr("%1 + Middle Drag:").arg(ctrlKeyName));
    ui->altVerticalScrollLabel->setText(tr("%1 + Vertical Scroll:").arg(ctrlKeyName));
    ui->altHorizontalScrollLabel->setText(tr("%1 + Horizontal Scroll:").arg(ctrlKeyName));

    syncSettings(false, true);
    syncShortcuts();
    syncFormats();
    updateButtonBox();

    isInitialLoad = false;
}

QVOptionsDialog::~QVOptionsDialog()
{
    delete ui;
}

void QVOptionsDialog::done(int r)
{
    // Save window geometry
    QSettings settings;
    settings.setValue("optionsgeometry", saveGeometry());
    settings.setValue("optionstab", ui->categoryList->currentRow());

    QDialog::done(r);
}

void QVOptionsDialog::modifySetting(QString key, QVariant value)
{
    transientSettings.insert(key, value);
    updateButtonBox();
}

void QVOptionsDialog::saveSettings()
{
    QSettings settings;

    settings.beginGroup("options");

    const auto keys = transientSettings.keys();
    for (const auto &key : keys)
    {
        const auto &value = transientSettings[key];
        settings.setValue(key, value);
    }

    settings.setValue("disabledfileextensions", Qv::setToList(transientDisabledFileExtensions).join(';'));

    settings.endGroup();

    settings.beginGroup("shortcuts");

    const auto &shortcutsList = qvApp->getShortcutManager().getShortcutsList();
    for (int i = 0; i < transientShortcuts.length(); i++)
    {
        settings.setValue(shortcutsList.value(i).name, transientShortcuts.value(i));
    }

    qvApp->getShortcutManager().updateShortcuts();
    qvApp->getSettingsManager().loadSettings();
}

void QVOptionsDialog::syncSettings(bool defaults, bool makeConnections)
{
    auto &settingsManager = qvApp->getSettingsManager();
    settingsManager.loadSettings();

    // bgcolorenabled
    syncCheckbox(ui->bgColorCheckbox, "bgcolorenabled", defaults, makeConnections);
    if (ui->bgColorCheckbox->isChecked())
        ui->bgColorButton->setEnabled(true);
    else
        ui->bgColorButton->setEnabled(false);
    // bgcolor
    ui->bgColorButton->setText(settingsManager.getString("bgcolor", defaults));
    transientSettings.insert("bgcolor", ui->bgColorButton->text());
    updateBgColorButton();
    connect(ui->bgColorButton, &QPushButton::clicked, this, &QVOptionsDialog::bgColorButtonClicked);
    // checkerboardbackground
    syncCheckbox(ui->checkerboardBackgroundCheckbox, "checkerboardbackground", defaults, makeConnections);
    // titlebarmode
    syncComboBox(ui->titlebarComboBox, "titlebarmode", defaults, makeConnections);
    // customtitlebartext
    syncLineEdit(ui->customTitlebarLineEdit, "customtitlebartext", defaults, makeConnections);
    // windowresizemode
    syncComboBox(ui->windowResizeComboBox, "windowresizemode", defaults, makeConnections);
    // aftermatchingsize
    syncComboBox(ui->afterMatchingSizeComboBox, "aftermatchingsizemode", defaults, makeConnections);
    // minwindowresizedpercentage
    syncSpinBox(ui->minWindowResizeSpinBox, "minwindowresizedpercentage", defaults, makeConnections);
    // maxwindowresizedperecentage
    syncSpinBox(ui->maxWindowResizeSpinBox, "maxwindowresizedpercentage", defaults, makeConnections);
    // nonnativetheme
    syncCheckbox(ui->nonNativeThemeCheckbox, "nonnativetheme", defaults, makeConnections);
    // titlebaralwaysdark
    syncCheckbox(ui->darkTitlebarCheckbox, "titlebaralwaysdark", defaults, makeConnections);
    // quitonlastwindow
    syncCheckbox(ui->quitOnLastWindowCheckbox, "quitonlastwindow", defaults, makeConnections);
    // menubarenabled
    syncCheckbox(ui->menubarCheckbox, "menubarenabled", defaults, makeConnections);
    // fullscreendetails
    syncCheckbox(ui->detailsInFullscreen, "fullscreendetails", defaults, makeConnections);
    // submenuicons
    syncCheckbox(ui->submenuIconsCheckbox, "submenuicons", defaults, makeConnections);
    // persistsession
    syncCheckbox(ui->persistSessionCheckbox, "persistsession", defaults, makeConnections);
    // filteringenabled
    syncCheckbox(ui->filteringCheckbox, "filteringenabled", defaults, makeConnections);
    // scalingenabled
    syncCheckbox(ui->scalingCheckbox, "scalingenabled", defaults, makeConnections);
    if (ui->scalingCheckbox->isChecked())
        ui->scalingTwoCheckbox->setEnabled(true);
    else
        ui->scalingTwoCheckbox->setEnabled(false);
    // scalingtwoenabled
    syncCheckbox(ui->scalingTwoCheckbox, "scalingtwoenabled", defaults, makeConnections);
    // scalefactor
    syncSpinBox(ui->scaleFactorSpinBox, "scalefactor", defaults, makeConnections);
    // cursorzoom
    syncCheckbox(ui->cursorZoomCheckbox, "cursorzoom", defaults, makeConnections);
    // onetoonepixelsizing
    syncCheckbox(ui->oneToOnePixelSizingCheckbox, "onetoonepixelsizing", defaults, makeConnections);
    // calculatedzoommode
    syncComboBox(ui->zoomDefaultComboBox, "calculatedzoommode", defaults, makeConnections);
    // pastactualsizeenabled
    syncCheckbox(ui->pastActualSizeCheckbox, "pastactualsizeenabled", defaults, makeConnections);
    // fitoverscan
    syncSpinBox(ui->fitOverscanSpinBox, "fitoverscan", defaults, makeConnections);
    // constrainimageposition
    syncCheckbox(ui->constrainImagePositionCheckbox, "constrainimageposition", defaults, makeConnections);
    if (ui->constrainImagePositionCheckbox->isChecked())
        ui->constrainCentersSmallImageCheckbox->setEnabled(true);
    else
        ui->constrainCentersSmallImageCheckbox->setEnabled(false);
    // constraincentersmallimage
    syncCheckbox(ui->constrainCentersSmallImageCheckbox, "constraincentersmallimage", defaults, makeConnections);
    // colorspaceconversion
    syncComboBox(ui->colorSpaceConversionComboBox, "colorspaceconversion", defaults, makeConnections);
    // language
    syncComboBox(ui->langComboBox, "language", defaults, makeConnections);
    // sortmode
    syncComboBox(ui->sortComboBox, "sortmode", defaults, makeConnections);
    // sortdescending
    syncRadioButtons({ui->descendingRadioButton0, ui->descendingRadioButton1}, "sortdescending", defaults, makeConnections);
    // preloadingmode
    syncComboBox(ui->preloadingComboBox, "preloadingmode", defaults, makeConnections);
    // loopfolders
    syncCheckbox(ui->loopFoldersCheckbox, "loopfoldersenabled", defaults, makeConnections);
    // slideshowdirection
    syncComboBox(ui->slideshowDirectionComboBox, "slideshowdirection", defaults, makeConnections);
    // slideshowtimer
    syncDoubleSpinBox(ui->slideshowTimerSpinBox, "slideshowtimer", defaults, makeConnections);
    // afterdelete
    syncComboBox(ui->afterDeletionComboBox, "afterdelete", defaults, makeConnections);
    // askdelete
    syncCheckbox(ui->askDeleteCheckbox, "askdelete", defaults, makeConnections);
    // allowmimecontentdetection
    syncCheckbox(ui->mimeContentDetectionCheckbox, "allowmimecontentdetection", defaults, makeConnections);
    // saverecents
    syncCheckbox(ui->saveRecentsCheckbox, "saverecents", defaults, makeConnections);
    // updatenotifications
    syncCheckbox(ui->updateCheckbox, "updatenotifications", defaults, makeConnections);

    // mouse actions
    syncComboBox(ui->doubleClickComboBox, "viewportdoubleclickaction", defaults, makeConnections);
    syncComboBox(ui->altDoubleClickComboBox, "viewportaltdoubleclickaction", defaults, makeConnections);
    syncComboBox(ui->dragComboBox, "viewportdragaction", defaults, makeConnections);
    syncComboBox(ui->altDragComboBox, "viewportaltdragaction", defaults, makeConnections);
    syncRadioButtons({ui->middleButtonModeClickRadioButton, ui->middleButtonModeDragRadioButton}, "viewportmiddlebuttonmode", defaults, makeConnections);
    middleButtonModeChanged();
    syncComboBox(ui->middleClickComboBox, "viewportmiddleclickaction", defaults, makeConnections);
    syncComboBox(ui->altMiddleClickComboBox, "viewportaltmiddleclickaction", defaults, makeConnections);
    syncComboBox(ui->middleDragComboBox, "viewportmiddledragaction", defaults, makeConnections);
    syncComboBox(ui->altMiddleDragComboBox, "viewportaltmiddledragaction", defaults, makeConnections);
    syncComboBox(ui->verticalScrollComboBox, "viewportverticalscrollaction", defaults, makeConnections);
    syncComboBox(ui->horizontalScrollComboBox, "viewporthorizontalscrollaction", defaults, makeConnections);
    syncComboBox(ui->altVerticalScrollComboBox, "viewportaltverticalscrollaction", defaults, makeConnections);
    syncComboBox(ui->altHorizontalScrollComboBox, "viewportalthorizontalscrollaction", defaults, makeConnections);
    syncCheckbox(ui->scrollActionCooldownCheckbox, "scrollactioncooldown", defaults, makeConnections);
}

void QVOptionsDialog::syncCheckbox(QCheckBox *checkbox, const QString &key, bool defaults, bool makeConnection)
{
    auto val = qvApp->getSettingsManager().getBoolean(key, defaults);
    checkbox->setChecked(val);
    transientSettings.insert(key, val);

    if (makeConnection)
    {
        connect(checkbox, &QCheckBox::stateChanged, this, [this, key](int state) {
            modifySetting(key, static_cast<bool>(state));
        });
    }
}

void QVOptionsDialog::syncRadioButtons(QList<QRadioButton *> buttons, const QString &key, bool defaults, bool makeConnection)
{
    auto val = qvApp->getSettingsManager().getInteger(key, defaults);
    if (auto widget = buttons.value(val))
        widget->setChecked(true);
    transientSettings.insert(key, val);

    if (makeConnection)
    {
        for (int i = 0; i < buttons.length(); i++)
        {
            connect(buttons.value(i), &QRadioButton::clicked, this, [this, key, i] {
                modifySetting(key, i);
            });
        }
    }
}

void QVOptionsDialog::syncComboBox(QComboBox *comboBox, const QString &key, bool defaults, bool makeConnection)
{
    auto val = qvApp->getSettingsManager().getString(key, defaults);
    comboBox->setCurrentIndex(comboBox->findData(val));
    transientSettings.insert(key, val);

    if (makeConnection)
    {
        connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, key, comboBox](int index) {
            Q_UNUSED(index)
            modifySetting(key, comboBox->currentData());
        });
    }
}

void QVOptionsDialog::syncSpinBox(QSpinBox *spinBox, const QString &key, bool defaults, bool makeConnection)
{
    auto val = qvApp->getSettingsManager().getInteger(key, defaults);
    spinBox->setValue(val);
    transientSettings.insert(key, val);

    if (makeConnection)
    {
        connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, key](int i) {
            modifySetting(key, i);
        });
    }
}

void QVOptionsDialog::syncDoubleSpinBox(QDoubleSpinBox *doubleSpinBox, const QString &key, bool defaults, bool makeConnection)
{
    auto val = qvApp->getSettingsManager().getDouble(key, defaults);
    doubleSpinBox->setValue(val);
    transientSettings.insert(key, val);

    if (makeConnection)
    {
        connect(doubleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, key](double d) {
            modifySetting(key, d);
        });
    }
}

void QVOptionsDialog::syncLineEdit(QLineEdit *lineEdit, const QString &key, bool defaults, bool makeConnection)
{
    auto val = qvApp->getSettingsManager().getString(key, defaults);
    lineEdit->setText(val);
    transientSettings.insert(key, val);

    if (makeConnection)
    {
        connect(lineEdit, &QLineEdit::textEdited, this, [this, key](const QString &text) {
            modifySetting(key, text);
        });
    }
}

void QVOptionsDialog::syncShortcuts(bool defaults)
{
    qvApp->getShortcutManager().updateShortcuts();

    transientShortcuts.clear();
    const auto &shortcutsList = qvApp->getShortcutManager().getShortcutsList();
    ui->shortcutsTable->setRowCount(shortcutsList.length());

    for (int i = 0; i < shortcutsList.length(); i++)
    {
        const ShortcutManager::SShortcut &shortcut = shortcutsList.value(i);

        // Add shortcut to transient shortcut list
        if (defaults)
            transientShortcuts.append(shortcut.defaultShortcuts);
        else
            transientShortcuts.append(shortcut.shortcuts);

        // Add shortcut to table widget
        auto *nameItem = new QTableWidgetItem();
        nameItem->setText(shortcut.readableName);
        ui->shortcutsTable->setItem(i, 0, nameItem);

        auto *shortcutsItem = new QTableWidgetItem();
        shortcutsItem->setText(ShortcutManager::stringListToReadableString(
                                   transientShortcuts.value(i)));
        ui->shortcutsTable->setItem(i, 1, shortcutsItem);
    }
    updateShortcutsTable();
}

void QVOptionsDialog::updateShortcutsTable()
{
    for (int i = 0; i < transientShortcuts.length(); i++)
    {
        const QStringList &shortcuts = transientShortcuts.value(i);
        ui->shortcutsTable->item(i, 1)->setText(ShortcutManager::stringListToReadableString(shortcuts));
    }
    updateButtonBox();
}

void QVOptionsDialog::shortcutCellDoubleClicked(int row, int column)
{
    Q_UNUSED(column)
    auto getTransientShortcutCallback = [this](int index) {
        return transientShortcuts.value(index);
    };
    auto *shortcutDialog = new QVShortcutDialog(row, getTransientShortcutCallback, this);
    connect(shortcutDialog, &QVShortcutDialog::shortcutsListChanged, this, [this](int index, const QStringList &stringListShortcuts) {
        transientShortcuts.replace(index, stringListShortcuts);
        updateShortcutsTable();
    });
    shortcutDialog->open();
}

void QVOptionsDialog::syncFormats(bool defaults)
{
    if (defaults)
        transientDisabledFileExtensions.clear();
    else
        transientDisabledFileExtensions = qvApp->getDisabledFileExtensions();

    QStringList extensions = qvApp->getAllFileExtensionList();
    extensions.sort(Qt::CaseInsensitive);

    isLoadingFormats = true;

    ui->formatsTable->setRowCount(extensions.length());

    for (int i = 0; i < extensions.length(); i++)
    {
        const QString &extension = extensions.value(i);
        const bool isDisabled = transientDisabledFileExtensions.contains(extension);

        auto *extensionItem = new QTableWidgetItem();
        extensionItem->setText(extension);
        ui->formatsTable->setItem(i, 0, extensionItem);

        auto *enabledItem = new QTableWidgetItem();
        enabledItem->setCheckState(isDisabled ? Qt::Unchecked : Qt::Checked);
        enabledItem->setData(Qt::UserRole, extension);
        ui->formatsTable->setItem(i, 1, enabledItem);
    }

    isLoadingFormats = false;
}

void QVOptionsDialog::buttonBoxClicked(QAbstractButton *button)
{
    auto role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::AcceptRole || role == QDialogButtonBox::ApplyRole)
    {
        saveSettings();
        if (role == QDialogButtonBox::ApplyRole)
            button->setEnabled(false);
    }
    else if (role == QDialogButtonBox::ResetRole)
    {
        syncSettings(true);
        syncShortcuts(true);
        syncFormats(true);
    }
}

void QVOptionsDialog::updateButtonBox()
{
    QPushButton *defaultsButton = ui->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    QPushButton *applyButton = ui->buttonBox->button(QDialogButtonBox::Apply);
    bool anyUnsaved = false;
    bool anyNonDefault = false;

    // settings
    const QList<QString> settingKeys = transientSettings.keys();
    for (const auto &key : settingKeys)
    {
        const auto &transientValue = transientSettings.value(key);
        const auto &savedValue = qvApp->getSettingsManager().getSetting(key);
        const auto &defaultValue = qvApp->getSettingsManager().getSetting(key, true);

        anyUnsaved |= transientValue != savedValue;
        anyNonDefault |= transientValue != defaultValue;
    }

    // shortcuts
    const QList<ShortcutManager::SShortcut> &shortcutsList = qvApp->getShortcutManager().getShortcutsList();
    for (int i = 0; i < transientShortcuts.length(); i++)
    {
        const auto &transientValue = transientShortcuts.value(i);
        QStringList savedValue = shortcutsList.value(i).shortcuts;
        QStringList defaultValue = shortcutsList.value(i).defaultShortcuts;

        anyUnsaved |= transientValue != savedValue;
        anyNonDefault |= transientValue != defaultValue;
    }

    // formats
    anyUnsaved |= transientDisabledFileExtensions != qvApp->getDisabledFileExtensions();
    anyNonDefault |= !transientDisabledFileExtensions.isEmpty();

    applyButton->setEnabled(anyUnsaved);
    defaultsButton->setEnabled(anyNonDefault);
}

void QVOptionsDialog::bgColorButtonClicked()
{

    auto *colorDialog = new QColorDialog(ui->bgColorButton->text(), this);
    colorDialog->setWindowModality(Qt::WindowModal);
    connect(colorDialog, &QDialog::accepted, colorDialog, [this, colorDialog] {
        auto selectedColor = colorDialog->currentColor();

        if (!selectedColor.isValid())
            return;

        modifySetting("bgcolor", selectedColor.name());
        ui->bgColorButton->setText(selectedColor.name());
        updateBgColorButton();
        colorDialog->deleteLater();
    });
    colorDialog->open();
}

void QVOptionsDialog::updateBgColorButton()
{
    QPixmap newPixmap = QPixmap(32, 32);
    newPixmap.fill(ui->bgColorButton->text());
    ui->bgColorButton->setIcon(QIcon(newPixmap));
}

void QVOptionsDialog::bgColorCheckboxStateChanged(int state)
{
    ui->bgColorButton->setEnabled(static_cast<bool>(state));
    updateBgColorButton();
}

void QVOptionsDialog::restartNotifyForCheckbox(const QString &key, const int state)
{
    const bool savedValue = qvApp->getSettingsManager().getBoolean(key);
    if (static_cast<bool>(state) != savedValue)
        QMessageBox::information(this, tr("Restart Required"), tr("You must restart qView to change this setting."));
}

void QVOptionsDialog::scalingCheckboxStateChanged(int state)
{
    ui->scalingTwoCheckbox->setEnabled(static_cast<bool>(state));
}

void QVOptionsDialog::titlebarComboBoxCurrentIndexChanged(int index)
{
    const auto value = static_cast<Qv::TitleBarText>(ui->titlebarComboBox->itemData(index).toInt());
    ui->customTitlebarLineEdit->setEnabled(value == Qv::TitleBarText::Custom);
}

void QVOptionsDialog::windowResizeComboBoxCurrentIndexChanged(int index)
{
    const auto value = static_cast<Qv::WindowResizeMode>(ui->windowResizeComboBox->itemData(index).toInt());
    bool enableRelatedControls = value != Qv::WindowResizeMode::Never;
    ui->afterMatchingSizeLabel->setEnabled(enableRelatedControls);
    ui->afterMatchingSizeComboBox->setEnabled(enableRelatedControls);
    ui->minWindowResizeLabel->setEnabled(enableRelatedControls);
    ui->minWindowResizeSpinBox->setEnabled(enableRelatedControls);
    ui->maxWindowResizeLabel->setEnabled(enableRelatedControls);
    ui->maxWindowResizeSpinBox->setEnabled(enableRelatedControls);
}

void QVOptionsDialog::constrainImagePositionCheckboxStateChanged(int state)
{
    ui->constrainCentersSmallImageCheckbox->setEnabled(static_cast<bool>(state));
}

void QVOptionsDialog::populateCategories()
{
    const int iconSize = 24;
    const int listRightPadding = 3;
    auto addItem = [&](const QChar &iconChar, const QString &text) {
        ui->categoryList->addItem(new QListWidgetItem(qvApp->iconFromFont("Material Icons Outlined", iconChar, iconSize, devicePixelRatioF()), text));
    };
    ui->categoryList->setIconSize(QSize(iconSize, iconSize));
    ui->categoryList->setFont(QApplication::font());
    addItem(u'\ue069', tr("Window"));
    addItem(u'\ue3f4', tr("Image"));
    addItem(u'\ue429', tr("Miscellaneous"));
    addItem(u'\ue312', tr("Shortcuts"));
    addItem(u'\ue323', tr("Mouse"));
    addItem(u'\ue87b', tr("Formats"));
    ui->categoryList->setFixedWidth(ui->categoryList->sizeHintForColumn(0) + ui->categoryList->frameWidth() + listRightPadding);
}

void QVOptionsDialog::populateLanguages()
{
    ui->langComboBox->clear();

    ui->langComboBox->addItem(tr("System Language"), "system");

    // Put english at the top seperately because it has no file
    ui->langComboBox->addItem("English (en)", "en");

    const auto entries = QDir(":/i18n/").entryList();
    for (auto entry : entries)
    {
        entry.remove(0, 6);
        entry.remove(entry.length()-3, 3);
        QLocale locale(entry);

        const QString langString = locale.nativeLanguageName() + " (" + entry + ")";

        ui->langComboBox->addItem(langString, entry);
    }
}

void QVOptionsDialog::languageComboBoxCurrentIndexChanged(int index)
{
    Q_UNUSED(index)
    if (!isInitialLoad && !languageRestartMessageShown)
    {
        QMessageBox::information(this, tr("Restart Required"), tr("You must restart qView to change the language."));
        languageRestartMessageShown = true;
    }
}

void QVOptionsDialog::formatsItemChanged(QTableWidgetItem *item)
{
    if (isLoadingFormats)
        return;

    const QString extension = item->data(Qt::UserRole).toString();
    if (item->checkState() == Qt::Unchecked)
        transientDisabledFileExtensions.insert(extension);
    else
        transientDisabledFileExtensions.remove(extension);

    updateButtonBox();
}

void QVOptionsDialog::middleButtonModeChanged()
{
    const bool isClick = ui->middleButtonModeClickRadioButton->isChecked();
    const bool isDrag = ui->middleButtonModeDragRadioButton->isChecked();
    ui->middleClickLabel->setVisible(isClick);
    ui->middleClickComboBox->setVisible(isClick);
    ui->altMiddleClickLabel->setVisible(isClick);
    ui->altMiddleClickComboBox->setVisible(isClick);
    ui->middleDragLabel->setVisible(isDrag);
    ui->middleDragComboBox->setVisible(isDrag);
    ui->altMiddleDragLabel->setVisible(isDrag);
    ui->altMiddleDragComboBox->setVisible(isDrag);
}

const Ui::ComboBoxItems<Qv::AfterDelete> QVOptionsDialog::mapAfterDelete() {
    return {
        { Qv::AfterDelete::MoveBack, tr("Move Back") },
        { Qv::AfterDelete::DoNothing, tr("Do Nothing") },
        { Qv::AfterDelete::MoveForward, tr("Move Forward") }
    };
}

const Ui::ComboBoxItems<Qv::AfterMatchingSize> QVOptionsDialog::mapAfterMatchingSize() {
    return {
        { Qv::AfterMatchingSize::AvoidRepositioning, tr("Avoid repositioning") },
        { Qv::AfterMatchingSize::CenterOnPrevious, tr("Center relative to previous image") },
        { Qv::AfterMatchingSize::CenterOnScreen, tr("Center relative to screen") }
    };
}

const Ui::ComboBoxItems<Qv::CalculatedZoomMode> QVOptionsDialog::mapCalculatedZoomMode() {
    return {
        { Qv::CalculatedZoomMode::ZoomToFit, tr("Zoom to Fit") },
        { Qv::CalculatedZoomMode::FillWindow, tr("Fill Window") }
    };
}

const Ui::ComboBoxItems<Qv::ColorSpaceConversion> QVOptionsDialog::mapColorSpaceConversion() {
    return {
        { Qv::ColorSpaceConversion::Disabled, tr("Disabled") },
        { Qv::ColorSpaceConversion::AutoDetect, tr("Auto-detect") },
        { Qv::ColorSpaceConversion::SRgb, tr("sRGB") },
        { Qv::ColorSpaceConversion::DisplayP3, tr("Display P3") }
    };
}

const Ui::ComboBoxItems<Qv::PreloadMode> QVOptionsDialog::mapPreloadMode() {
    return {
        { Qv::PreloadMode::Disabled, tr("Disabled") },
        { Qv::PreloadMode::Adjacent, tr("Adjacent") },
        { Qv::PreloadMode::Extended, tr("Extended") }
    };
}

const Ui::ComboBoxItems<Qv::SlideshowDirection> QVOptionsDialog::mapSlideshowDirection() {
    return {
        { Qv::SlideshowDirection::Forward, tr("Forward") },
        { Qv::SlideshowDirection::Backward, tr("Backward") },
        { Qv::SlideshowDirection::Random, tr("Random") }
    };
}

const Ui::ComboBoxItems<Qv::SortMode> QVOptionsDialog::mapSortMode() {
    return {
        { Qv::SortMode::Name, tr("Name") },
        { Qv::SortMode::DateModified, tr("Date Modified") },
        { Qv::SortMode::DateCreated, tr("Date Created") },
        { Qv::SortMode::Size, tr("Size") },
        { Qv::SortMode::Type, tr("Type") },
        { Qv::SortMode::Random, tr("Random") }
    };
}

const Ui::ComboBoxItems<Qv::TitleBarText> QVOptionsDialog::mapTitleBarText() {
    return {
        { Qv::TitleBarText::Basic, tr("Basic") },
        { Qv::TitleBarText::Minimal, tr("Minimal") },
        { Qv::TitleBarText::Practical, tr("Practical") },
        { Qv::TitleBarText::Verbose, tr("Verbose") },
        { Qv::TitleBarText::Custom, tr("Custom") }
    };
}

const Ui::ComboBoxItems<Qv::WindowResizeMode> QVOptionsDialog::mapWindowResizeMode() {
    return {
        { Qv::WindowResizeMode::Never, tr("Never") },
        { Qv::WindowResizeMode::WhenLaunching, tr("When launching") },
        { Qv::WindowResizeMode::WhenOpeningImages, tr("When opening images") }
    };
}

const Ui::ComboBoxItems<Qv::ViewportClickAction> QVOptionsDialog::mapViewportClickAction() {
    return {
        { Qv::ViewportClickAction::None, tr("None") },
        { Qv::ViewportClickAction::ZoomToFit, tr("Zoom to Fit") },
        { Qv::ViewportClickAction::FillWindow, tr("Fill Window") },
        { Qv::ViewportClickAction::OriginalSize, tr("Original Size") },
        { Qv::ViewportClickAction::ToggleFullScreen, tr("Toggle Full Screen") },
        { Qv::ViewportClickAction::ToggleTitlebarHidden, tr("Toggle Titlebar Hidden") }
    };
}

const Ui::ComboBoxItems<Qv::ViewportDragAction> QVOptionsDialog::mapViewportDragAction() {
    return {
        { Qv::ViewportDragAction::None, tr("None") },
        { Qv::ViewportDragAction::Pan, tr("Pan") },
        { Qv::ViewportDragAction::MoveWindow, tr("Move Window") }
    };
}

const Ui::ComboBoxItems<Qv::ViewportScrollAction> QVOptionsDialog::mapViewportScrollAction() {
    return {
        { Qv::ViewportScrollAction::None, tr("None") },
        { Qv::ViewportScrollAction::Zoom, tr("Zoom") },
        { Qv::ViewportScrollAction::Navigate, tr("Navigate") },
        { Qv::ViewportScrollAction::Pan, tr("Pan") }
    };
}

template <typename TEnum>
static void populateComboBox(QComboBox *comboBox, const Ui::ComboBoxItems<TEnum> &items)
{
    comboBox->clear();
    for (const auto &item : items)
    {
        comboBox->addItem(item.second, static_cast<int>(item.first));
    }
}

void QVOptionsDialog::populateComboBoxes()
{
    populateComboBox(ui->titlebarComboBox, mapTitleBarText());

    populateComboBox(ui->windowResizeComboBox, mapWindowResizeMode());

    populateComboBox(ui->afterMatchingSizeComboBox, mapAfterMatchingSize());

    populateComboBox(ui->zoomDefaultComboBox, mapCalculatedZoomMode());

    populateComboBox(ui->colorSpaceConversionComboBox, mapColorSpaceConversion());

    populateComboBox(ui->sortComboBox, mapSortMode());

    populateComboBox(ui->preloadingComboBox, mapPreloadMode());

    populateComboBox(ui->slideshowDirectionComboBox, mapSlideshowDirection());

    populateComboBox(ui->afterDeletionComboBox, mapAfterDelete());

    populateComboBox(ui->doubleClickComboBox, mapViewportClickAction());
    populateComboBox(ui->altDoubleClickComboBox, mapViewportClickAction());
    populateComboBox(ui->middleClickComboBox, mapViewportClickAction());
    populateComboBox(ui->altMiddleClickComboBox, mapViewportClickAction());

    populateComboBox(ui->dragComboBox, mapViewportDragAction());
    populateComboBox(ui->altDragComboBox, mapViewportDragAction());
    populateComboBox(ui->middleDragComboBox, mapViewportDragAction());
    populateComboBox(ui->altMiddleDragComboBox, mapViewportDragAction());

    populateComboBox(ui->verticalScrollComboBox, mapViewportScrollAction());
    populateComboBox(ui->horizontalScrollComboBox, mapViewportScrollAction());
    populateComboBox(ui->altVerticalScrollComboBox, mapViewportScrollAction());
    populateComboBox(ui->altHorizontalScrollComboBox, mapViewportScrollAction());
}
