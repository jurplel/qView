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
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    #ifdef Q_OS_UNIX
    setWindowTitle("Preferences");
    #endif

    #ifdef Q_OS_MACX
        ui->menubarCheckbox->hide();
    #endif

    loadSettings();
    loadShortcuts();
}

QVOptionsDialog::~QVOptionsDialog()
{
    delete ui;
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
}

void QVOptionsDialog::loadSettings(bool defaults)
{
    auto &settingsManager = qvApp->getSettingsManager();
    settingsManager.updateSettings();

    // bgcolorenabled
    syncCheckbox(ui->bgColorCheckbox, "bgcolorenabled", defaults);
    if (ui->bgColorCheckbox->isChecked())
        ui->bgColorButton->setEnabled(true);
    else
        ui->bgColorButton->setEnabled(false);

    // bgcolor
    ui->bgColorButton->setText(settingsManager.getString("bgcolor", defaults));
    transientSettings.insert("bgcolor", ui->bgColorButton->text());
    updateBgColorButton();

    // titlebarmode
    syncRadioButtons({ui->titlebarRadioButton0, ui->titlebarRadioButton1,
                     ui->titlebarRadioButton2, ui->titlebarRadioButton3}, "titlebarmode", defaults);

    // windowresizemode
    syncComboBox(ui->windowResizeComboBox, "windowresizemode", defaults);
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
    syncSpinBox(ui->minWindowResizeSpinBox, "minwindowresizedpercentage", defaults);

    // maxwindowresizedperecentage
    syncSpinBox(ui->maxWindowResizeSpinBox, "maxwindowresizedpercentage", defaults);

    // menubarenabled
    syncCheckbox(ui->menubarCheckbox, "menubarenabled", defaults);

    // filteringenabled
    syncCheckbox(ui->filteringCheckbox, "filteringenabled", defaults);

    // scalingenabled
    syncCheckbox(ui->scalingCheckbox, "scalingenabled", defaults);
    if (ui->scalingCheckbox->isChecked())
        ui->scalingTwoCheckbox->setEnabled(true);
    else
        ui->scalingTwoCheckbox->setEnabled(false);

    // scalingtwoenabled
    syncCheckbox(ui->scalingTwoCheckbox, "scalingtwoenabled", defaults);

    // scalefactor
    syncSpinBox(ui->scaleFactorSpinBox, "scalefactor", defaults);

    // scrollzoomsenabled
    syncCheckbox(ui->scrollZoomsCheckbox, "scrollzoomsenabled", defaults);

    // cursorzoom
    syncCheckbox(ui->cursorZoomCheckbox, "cursorzoom", defaults);

    // cropmode
    syncComboBox(ui->cropModeComboBox, "cropmode", defaults);

    // pastactualsizeenabled
    syncCheckbox(ui->pastActualSizeCheckbox, "pastactualsizeenabled", defaults);

    // sortmode
    syncComboBox(ui->sortComboBox, "sortmode", defaults);

    // sortascending
    syncRadioButtons({ui->ascendingRadioButton0, ui->ascendingRadioButton1}, "sortascending", defaults);

    // preloadingmode
    syncComboBox(ui->preloadingComboBox, "preloadingmode", defaults);

    // loopfolders
    syncCheckbox(ui->loopFoldersCheckbox, "loopfoldersenabled", defaults);

    // slideshowreversed
    syncComboBox(ui->slideshowDirectionComboBox, "slideshowreversed", defaults);

    // slideshowtimer
    syncDoubleSpinBox(ui->slideshowTimerSpinBox, "slideshowtimer", defaults);

    // saverecents
    syncCheckbox(ui->saveRecentsCheckbox, "saverecents", defaults);
}

void QVOptionsDialog::syncCheckbox(QCheckBox *checkbox, const QString &key, bool defaults)
{
    auto val = qvApp->getSettingsManager().getBoolean(key, defaults);
    checkbox->setChecked(val);
    transientSettings.insert(key, val);
}

void QVOptionsDialog::syncRadioButtons(QList<QRadioButton *> buttons, const QString &key, bool defaults)
{
    auto val = qvApp->getSettingsManager().getInteger(key, defaults);
    buttons.value(val)->setChecked(true);
    transientSettings.insert(key, val);
}

void QVOptionsDialog::syncComboBox(QComboBox *comboBox, const QString &key, bool defaults)
{
    auto val = qvApp->getSettingsManager().getInteger(key, defaults);
    comboBox->setCurrentIndex(val);
    transientSettings.insert(key, val);
}

void QVOptionsDialog::syncSpinBox(QSpinBox *spinBox, const QString &key, bool defaults)
{
    auto val = qvApp->getSettingsManager().getInteger(key, defaults);
    spinBox->setValue(val);
    transientSettings.insert(key, val);
}

void QVOptionsDialog::syncDoubleSpinBox(QDoubleSpinBox *doubleSpinBox, const QString &key, bool defaults)
{
    auto val = qvApp->getSettingsManager().getDouble(key, defaults);
    doubleSpinBox->setValue(val);
    transientSettings.insert(key, val);
}

void QVOptionsDialog::loadShortcuts(bool defaults)
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
    auto shortcutDialog = new QVShortcutDialog(row);
    shortcutDialog->open();
    connect(shortcutDialog, &QVShortcutDialog::shortcutsListChanged, [this](int index, const QStringList &stringListShortcuts){
        transientShortcuts.insert(index, stringListShortcuts);
        updateShortcutsTable();
    });
}

void QVOptionsDialog::updateBgColorButton()
{
    QPixmap newPixmap = QPixmap(32, 32);
    newPixmap.fill(ui->bgColorButton->text());
    ui->bgColorButton->setIcon(QIcon(newPixmap));
}

void QVOptionsDialog::on_bgColorButton_clicked()
{

    QColor selectedColor = QColorDialog::getColor(qvApp->getSettingsManager().getString("bgcolor"), this);

    if (!selectedColor.isValid())
        return;

    transientSettings.insert("bgcolor", selectedColor.name());
    ui->bgColorButton->setText(selectedColor.name());
    updateBgColorButton();
}

void QVOptionsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole || ui->buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole)
    {
        saveSettings();
    }
    else if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole)
    {
        loadSettings(true);
        loadShortcuts(true);
    }
}

void QVOptionsDialog::on_bgColorCheckbox_stateChanged(int arg1)
{
    transientSettings.insert("bgcolorenabled", static_cast<bool>(arg1));
    if (arg1 > 0)
        ui->bgColorButton->setEnabled(true);
    else
        ui->bgColorButton->setEnabled(false);

    updateBgColorButton();
}

void QVOptionsDialog::on_filteringCheckbox_stateChanged(int arg1)
{
    transientSettings.insert("filteringenabled", static_cast<bool>(arg1));
}

void QVOptionsDialog::on_scalingCheckbox_stateChanged(int arg1)
{
    transientSettings.insert("scalingenabled", static_cast<bool>(arg1));
    if (arg1 > 0)
        ui->scalingTwoCheckbox->setEnabled(true);
    else
        ui->scalingTwoCheckbox->setEnabled(false);
}

void QVOptionsDialog::on_menubarCheckbox_stateChanged(int arg1)
{
    transientSettings.insert("menubarenabled", static_cast<bool>(arg1));
}

void QVOptionsDialog::on_cropModeComboBox_currentIndexChanged(int index)
{
    transientSettings.insert("cropmode", index);
}

void QVOptionsDialog::on_slideshowTimerSpinBox_valueChanged(double arg1)
{
    transientSettings.insert("slideshowtimer", arg1);
}

void QVOptionsDialog::on_slideshowDirectionComboBox_currentIndexChanged(int index)
{
    transientSettings.insert("slideshowreversed", static_cast<bool>(index));
}

void QVOptionsDialog::on_scaleFactorSpinBox_valueChanged(int arg1)
{
    transientSettings.insert("scalefactor", arg1);
}

void QVOptionsDialog::on_scalingTwoCheckbox_stateChanged(int arg1)
{
    transientSettings.insert("scalingtwoenabled", static_cast<bool>(arg1));
}

void QVOptionsDialog::on_pastActualSizeCheckbox_stateChanged(int arg1)
{
    transientSettings.insert("pastactualsizeenabled", static_cast<bool>(arg1));
}

void QVOptionsDialog::on_scrollZoomsCheckbox_stateChanged(int arg1)
{
    transientSettings.insert("scrollzoomsenabled", static_cast<bool>(arg1));
}

void QVOptionsDialog::on_titlebarRadioButton0_clicked()
{
    transientSettings.insert("titlebarmode", 0);
}

void QVOptionsDialog::on_titlebarRadioButton1_clicked()
{
    transientSettings.insert("titlebarmode", 1);
}

void QVOptionsDialog::on_titlebarRadioButton2_clicked()
{
    transientSettings.insert("titlebarmode", 2);
}

void QVOptionsDialog::on_titlebarRadioButton3_clicked()
{
    transientSettings.insert("titlebarmode", 3);
}

void QVOptionsDialog::on_windowResizeComboBox_currentIndexChanged(int index)
{
    transientSettings.insert("windowresizemode", index);
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

void QVOptionsDialog::on_minWindowResizeSpinBox_valueChanged(int arg1)
{
    transientSettings.insert("minwindowresizedpercentage", arg1);
}

void QVOptionsDialog::on_maxWindowResizeSpinBox_valueChanged(int arg1)
{
    transientSettings.insert("maxwindowresizedpercentage", arg1);
}

void QVOptionsDialog::on_loopFoldersCheckbox_stateChanged(int arg1)
{
    transientSettings.insert("loopfolders", static_cast<bool>(arg1));
}

void QVOptionsDialog::on_preloadingComboBox_currentIndexChanged(int index)
{
    transientSettings.insert("preloadingmode", index);
}

void QVOptionsDialog::on_sortComboBox_currentIndexChanged(int index)
{
    transientSettings.insert("sortmode", index);
}

void QVOptionsDialog::on_ascendingRadioButton0_clicked()
{
    transientSettings.insert("sortascending", true);
}

void QVOptionsDialog::on_ascendingRadioButton1_clicked()
{
    transientSettings.insert("sortascending", false);
}

void QVOptionsDialog::on_saveRecentsCheckbox_stateChanged(int arg1)
{
    transientSettings.insert("saverecents", static_cast<bool>(arg1));
}

void QVOptionsDialog::on_cursorZoomCheckbox_stateChanged(int arg1)
{
    transientSettings.insert("cursorzoom", static_cast<bool>(arg1));
}
