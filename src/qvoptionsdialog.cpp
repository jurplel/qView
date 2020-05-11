#include "qvoptionsdialog.h"
#include "ui_qvoptionsdialog.h"
#include "qvapplication.h"
#include <QColorDialog>
#include <QPalette>
#include <QScreen>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>

#include <QDebug>

QVOptionsDialog::QVOptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVOptionsDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::CustomizeWindowHint);

    // Load window geometry
    QSettings settings;
    restoreGeometry(settings.value("optionsgeometry").toByteArray());
    ui->tabWidget->setCurrentIndex(settings.value("optionstab", 1).toInt());

    #ifdef Q_OS_UNIX
    setWindowTitle("Preferences");
    #endif

    #ifdef Q_OS_MACOS
    ui->menubarCheckbox->hide();
    #else
    ui->darkTitlebarCheckbox->hide();
    #endif

    syncSettings(false, true);
    syncShortcuts();
}

QVOptionsDialog::~QVOptionsDialog()
{
    delete ui;
}

void QVOptionsDialog::closeEvent(QCloseEvent *event)
{
    if (isWindowModified())
    {
        QMessageBox *messageBox = new QMessageBox(QMessageBox::Question,
            tr("Unsaved Changes"), tr("Save changes before closing?"),
            QMessageBox::Discard | QMessageBox::Save | QMessageBox::Cancel, this);
        messageBox->setWindowModality(Qt::WindowModal);
        messageBox->setAttribute(Qt::WA_DeleteOnClose);
        messageBox->setDefaultButton(QMessageBox::Save);
        connect(messageBox, &QDialog::finished, [this, event](int result){
            switch(result) {
            case QMessageBox::StandardButton::Save: {
                saveSettings();
                actuallyClose(event);
                break;
            }
            case QMessageBox::StandardButton::Discard: {
                actuallyClose(event);
                break;
            }
            }
        });
        messageBox->open();
        event->ignore();
    }
    else
    {
        actuallyClose(event);
    }
}

void QVOptionsDialog::actuallyClose(QCloseEvent *event)
{
    // Save window geometry
    QSettings settings;
    settings.setValue("optionsgeometry", saveGeometry());
    settings.setValue("optionstab", ui->tabWidget->currentIndex());

    QDialog::closeEvent(event);
}

void QVOptionsDialog::modifySetting(QString key, QVariant value)
{
    transientSettings.insert(key, value);

    setWindowModified(false);
    const auto keys = transientSettings.keys();
    for (const auto &key : keys)
    {
        const auto &value0 = transientSettings.value(key);
        const auto &value1 = qvApp->getSettingsManager().getSetting(key);
        if (value1 != value0)
        {
            setWindowModified(true);
            break;
        }
    }
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

    settings.endGroup();
    settings.beginGroup("shortcuts");

    auto shortcutsList = qvApp->getActionManager().getShortcutsList();

    QHashIterator<int, QStringList> iter(transientShortcuts);
    while (iter.hasNext())
    {
        iter.next();
        settings.setValue(shortcutsList.value(iter.key()).name, iter.value());
    }

    qvApp->getActionManager().updateShortcuts();
    qvApp->getSettingsManager().updateSettings();

    setWindowModified(false);
}

void QVOptionsDialog::syncSettings(bool defaults, bool makeConnections)
{
    auto &settingsManager = qvApp->getSettingsManager();
    settingsManager.updateSettings();

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

    // titlebarmode
    syncRadioButtons({ui->titlebarRadioButton0, ui->titlebarRadioButton1,
                     ui->titlebarRadioButton2, ui->titlebarRadioButton3}, "titlebarmode", defaults, makeConnections);

    // windowresizemode
    syncComboBox(ui->windowResizeComboBox, "windowresizemode", defaults, makeConnections);
    if (ui->windowResizeComboBox->currentIndex() == 0)
    {
        ui->minWindowResizeLabel->setEnabled(false);
        ui->minWindowResizeSpinBox->setEnabled(false);
        ui->maxWindowResizeLabel->setEnabled(false);
        ui->maxWindowResizeSpinBox->setEnabled(false);
    }
    else
    {
        ui->minWindowResizeLabel->setEnabled(true);
        ui->minWindowResizeSpinBox->setEnabled(true);
        ui->maxWindowResizeLabel->setEnabled(true);
        ui->maxWindowResizeSpinBox->setEnabled(true);
    }

    // minwindowresizedpercentage
    syncSpinBox(ui->minWindowResizeSpinBox, "minwindowresizedpercentage", defaults, makeConnections);

    // maxwindowresizedperecentage
    syncSpinBox(ui->maxWindowResizeSpinBox, "maxwindowresizedpercentage", defaults, makeConnections);

    // titlebaralwaysdark
    syncCheckbox(ui->darkTitlebarCheckbox, "titlebaralwaysdark", defaults, makeConnections);

    // menubarenabled
    syncCheckbox(ui->menubarCheckbox, "menubarenabled", defaults, makeConnections);

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

    // scrollzoomsenabled
    syncCheckbox(ui->scrollZoomsCheckbox, "scrollzoomsenabled", defaults, makeConnections);

    // cursorzoom
    syncCheckbox(ui->cursorZoomCheckbox, "cursorzoom", defaults, makeConnections);

    // cropmode
    syncComboBox(ui->cropModeComboBox, "cropmode", defaults, makeConnections);

    // pastactualsizeenabled
    syncCheckbox(ui->pastActualSizeCheckbox, "pastactualsizeenabled", defaults, makeConnections);

    // sortmode
    syncComboBox(ui->sortComboBox, "sortmode", defaults, makeConnections);

    // sortascending
    syncRadioButtons({ui->ascendingRadioButton0, ui->ascendingRadioButton1}, "sortascending", defaults, makeConnections);

    // preloadingmode
    syncComboBox(ui->preloadingComboBox, "preloadingmode", defaults, makeConnections);

    // loopfolders
    syncCheckbox(ui->loopFoldersCheckbox, "loopfoldersenabled", defaults, makeConnections);

    // slideshowreversed
    syncComboBox(ui->slideshowDirectionComboBox, "slideshowreversed", defaults, makeConnections);

    // slideshowtimer
    syncDoubleSpinBox(ui->slideshowTimerSpinBox, "slideshowtimer", defaults, makeConnections);

    // saverecents
    syncCheckbox(ui->saveRecentsCheckbox, "saverecents", defaults, makeConnections);
}

void QVOptionsDialog::syncCheckbox(QCheckBox *checkbox, const QString &key, bool defaults, bool makeConnection)
{
    auto val = qvApp->getSettingsManager().getBoolean(key, defaults);
    checkbox->setChecked(val);
    transientSettings.insert(key, val);

    if (makeConnection)
    {
        connect(checkbox, &QCheckBox::stateChanged, [this, key](int arg1){
            modifySetting(key, static_cast<bool>(arg1));
        });
    }
}

void QVOptionsDialog::syncRadioButtons(QList<QRadioButton *> buttons, const QString &key, bool defaults, bool makeConnection)
{
    auto val = qvApp->getSettingsManager().getInteger(key, defaults);
    buttons.value(val)->setChecked(true);
    transientSettings.insert(key, val);

    if (makeConnection)
    {
        for (int i = 0; i < buttons.length(); i++)
        {
            connect(buttons.value(i), &QRadioButton::clicked, [this, key, i]{
                modifySetting(key, i);
            });
        }
    }
}

void QVOptionsDialog::syncComboBox(QComboBox *comboBox, const QString &key, bool defaults, bool makeConnection)
{
    auto val = qvApp->getSettingsManager().getInteger(key, defaults);
    comboBox->setCurrentIndex(val);
    transientSettings.insert(key, val);

    if (makeConnection)
    {
        connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, key](int index){
            modifySetting(key, index);
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
        connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this, key](int arg1){
            modifySetting(key, arg1);
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
        connect(doubleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, key](double arg1){
            modifySetting(key, arg1);
        });
    }
}

void QVOptionsDialog::syncShortcuts(bool defaults)
{
    qvApp->getActionManager().updateShortcuts();

    auto shortcutsList = qvApp->getActionManager().getShortcutsList();
    ui->shortcutsTable->setRowCount(shortcutsList.length());

    auto item = new QTableWidgetItem();

    int i = 0;
    QListIterator<ActionManager::SShortcut> iter(shortcutsList);
    while (iter.hasNext())
    {
        auto value = iter.next();
        if (defaults)
        {
            transientShortcuts.insert(i, value.defaultShortcuts);
        }
        else
        {
            transientShortcuts.insert(i, value.shortcuts);
        }

        item->setText(value.readableName);
        ui->shortcutsTable->setItem(i, 0, item->clone());

        item->setText(ActionManager::stringListToReadableString(transientShortcuts.value(i)));
        ui->shortcutsTable->setItem(i, 1, item->clone());
        i++;
    }
    delete item;
    updateShortcutsTable();
}

void QVOptionsDialog::updateShortcutsTable()
{
    QHashIterator<int, QStringList> iter(transientShortcuts);
    while (iter.hasNext())
    {
        iter.next();
        ui->shortcutsTable->item(iter.key(), 1)->setText(ActionManager::stringListToReadableString(iter.value()));
    }
}

void QVOptionsDialog::on_shortcutsTable_cellDoubleClicked(int row, int column)
{
    Q_UNUSED(column)
    auto shortcutDialog = new QVShortcutDialog(row, this);
    connect(shortcutDialog, &QVShortcutDialog::shortcutsListChanged, [this](int index, const QStringList &stringListShortcuts){
        transientShortcuts.insert(index, stringListShortcuts);
        updateShortcutsTable();
    });
    shortcutDialog->open();
}

void QVOptionsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole || ui->buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole)
    {
        saveSettings();
    }
    else if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole)
    {
        syncSettings(true);
        syncShortcuts(true);
    }
}

void QVOptionsDialog::bgColorButtonClicked()
{

    auto *colorDialog = new QColorDialog(ui->bgColorButton->text(), this);
    colorDialog->setWindowModality(Qt::WindowModal);
    connect(colorDialog, &QDialog::accepted, [this, colorDialog]{
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

void QVOptionsDialog::on_bgColorCheckbox_stateChanged(int arg1)
{
    if (arg1 > 0)
        ui->bgColorButton->setEnabled(true);
    else
        ui->bgColorButton->setEnabled(false);

    updateBgColorButton();
}

void QVOptionsDialog::on_scalingCheckbox_stateChanged(int arg1)
{
    if (arg1 > 0)
        ui->scalingTwoCheckbox->setEnabled(true);
    else
        ui->scalingTwoCheckbox->setEnabled(false);
}

void QVOptionsDialog::on_windowResizeComboBox_currentIndexChanged(int index)
{
    if (index == 0)
    {
        ui->minWindowResizeLabel->setEnabled(false);
        ui->minWindowResizeSpinBox->setEnabled(false);
        ui->maxWindowResizeLabel->setEnabled(false);
        ui->maxWindowResizeSpinBox->setEnabled(false);
    }
    else
    {
        ui->minWindowResizeLabel->setEnabled(true);
        ui->minWindowResizeSpinBox->setEnabled(true);
        ui->maxWindowResizeLabel->setEnabled(true);
        ui->maxWindowResizeSpinBox->setEnabled(true);
    }
}
