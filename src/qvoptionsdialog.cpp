#include "qvoptionsdialog.h"
#include "ui_qvoptionsdialog.h"
#include "qvapplication.h"
#include <QColorDialog>
#include <QPalette>
#include <QScreen>
#include <QMessageBox>

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
}

QVOptionsDialog::~QVOptionsDialog()
{
    delete ui;
}

void QVOptionsDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    loadSettings();
    loadShortcuts();
}

void QVOptionsDialog::saveSettings()
{
    QSettings settings;
    settings.beginGroup("options");

    settings.setValue("bgcolor", transientSettings.bgColor);
    settings.setValue("bgcolorenabled", transientSettings.bgColorEnabled);
    settings.setValue("filteringenabled", transientSettings.filteringEnabled);
    settings.setValue("scalingenabled", transientSettings.scalingEnabled);
    settings.setValue("titlebarmode", transientSettings.titlebarMode);
    settings.setValue("menubarenabled", transientSettings.menubarEnabled);
    settings.setValue("cropmode", transientSettings.cropMode);
    settings.setValue("slideshowtimer", transientSettings.slideshowTimer);
    settings.setValue("slideshowdirection", transientSettings.slideshowDirection);
    settings.setValue("scalefactor", transientSettings.scaleFactor);
    settings.setValue("scalingtwoenabled", transientSettings.scalingTwoEnabled);
    settings.setValue("pastactualsizeenabled", transientSettings.pastActualSizeEnabled);
    settings.setValue("scrollzoomsenabled", transientSettings.scrollZoomsEnabled);
    settings.setValue("windowresizemode", transientSettings.windowResizeMode);
    settings.setValue("maxwindowresizedpercentage", transientSettings.maxWindowResizedPercentage);
    settings.setValue("loopfoldersenabled", transientSettings.loopFoldersEnabled);
    settings.setValue("preloadingmode", transientSettings.preloadingMode);
    settings.setValue("sortmode", transientSettings.sortMode);
    settings.setValue("sortascending", transientSettings.sortAscending);
    settings.setValue("saverecents", transientSettings.saveRecents);
    settings.setValue("cursorzoom", transientSettings.cursorZoom);

    // Shortcuts are saved here too for now
    settings.endGroup();
    settings.beginGroup("shortcuts");

    auto shortcutsList = qvApp->getActionManager()->getShortcutsList();

    QHashIterator<int, QStringList> iter(transientShortcuts);
    while (iter.hasNext())
    {
        iter.next();
        settings.setValue(shortcutsList.value(iter.key()).name, iter.value());
    }
    qvApp->getActionManager()->updateShortcuts();

    emit optionsSaved();
}

void QVOptionsDialog::loadSettings(bool defaults)
{
    QSettings settings;
    if (!defaults)
        settings.beginGroup("options");
    else
        settings.beginGroup("empty");

    //bgcolor
    transientSettings.bgColor = settings.value("bgcolor", QString("#212121")).toString();
    ui->bgColorButton->setText(transientSettings.bgColor);
    updateBgColorButton();

    transientSettings.bgColorEnabled = settings.value("bgcolorenabled", true).toBool();
    if (transientSettings.bgColorEnabled)
    {
        ui->bgColorCheckbox->setChecked(true);
        ui->bgColorButton->setEnabled(true);
    }
    else
    {
        ui->bgColorCheckbox->setChecked(false);
        ui->bgColorButton->setEnabled(false);
    }

    //filtering
    transientSettings.filteringEnabled = settings.value("filteringenabled", true).toBool();
    ui->filteringCheckbox->setChecked(transientSettings.filteringEnabled);

    //scaling
    transientSettings.scalingEnabled = settings.value("scalingenabled", true).toBool();
    ui->scalingCheckbox->setChecked(transientSettings.scalingEnabled);
    if (!transientSettings.scalingEnabled)
    {
        ui->scalingTwoCheckbox->setEnabled(false);
    }
    else
    {
        ui->scalingTwoCheckbox->setEnabled(true);
    }

    //titlebar
    transientSettings.titlebarMode = settings.value("titlebarmode", 1).toInt();
    switch (transientSettings.titlebarMode)
    {
    case 0:
    {
        ui->titlebarRadioButton0->setChecked(true);
        break;
    }
    case 1:
    {
        ui->titlebarRadioButton1->setChecked(true);
        break;
    }
    case 2:
    {
        ui->titlebarRadioButton2->setChecked(true);
        break;
    }
    }

    //menubar
    transientSettings.menubarEnabled = settings.value("menubarenabled", false).toBool();
    ui->menubarCheckbox->setChecked(transientSettings.menubarEnabled);
    //hide menubar checkbox if on macOS, because it doesn't even work
    #ifdef Q_OS_MACX
    ui->menubarCheckbox->hide();
    #endif

    //cropmode
    transientSettings.cropMode = settings.value("cropmode", 0).toInt();
    ui->cropModeComboBox->setCurrentIndex(transientSettings.cropMode);

    //slideshowtimer
    transientSettings.slideshowTimer = settings.value("slideshowtimer", 5).toDouble();
    ui->slideshowTimerSpinBox->setValue(transientSettings.slideshowTimer);

    //slideshowdirection
    transientSettings.slideshowDirection = settings.value("slideshowdirection", 0).toInt();
    ui->slideshowDirectionComboBox->setCurrentIndex(transientSettings.slideshowDirection);

    //scalefactor
    transientSettings.scaleFactor = settings.value("scalefactor", 25).toInt();
    ui->scaleFactorSpinBox->setValue(transientSettings.scaleFactor);

    //scaling2 (while zooming in)
    transientSettings.scalingTwoEnabled = settings.value("scalingtwoenabled", true).toBool();
    ui->scalingTwoCheckbox->setChecked(transientSettings.scalingTwoEnabled);

    //resize past actual size
    transientSettings.pastActualSizeEnabled = settings.value("pastactualsizeenabled", true).toBool();
    ui->pastActualSizeCheckbox->setChecked(transientSettings.pastActualSizeEnabled);

    //resize past actual size
    transientSettings.scrollZoomsEnabled = settings.value("scrollzoomsenabled", true).toBool();
    ui->scrollZoomsCheckbox->setChecked(transientSettings.scrollZoomsEnabled);

    //window resize mode
    transientSettings.windowResizeMode = settings.value("windowresizemode", 1).toInt();
    ui->windowResizeComboBox->setCurrentIndex(transientSettings.windowResizeMode);
    if (transientSettings.windowResizeMode == 0)
    {
        ui->maxWindowResizeLabel->setEnabled(false);
        ui->maxWindowResizeSpinBox->setEnabled(false);
    }
    else
    {
        ui->maxWindowResizeLabel->setEnabled(true);
        ui->maxWindowResizeSpinBox->setEnabled(true);
    }

    //maximum size for auto window resize
    transientSettings.maxWindowResizedPercentage = settings.value("maxwindowresizedpercentage", 70).toInt();
    ui->maxWindowResizeSpinBox->setValue(transientSettings.maxWindowResizedPercentage);

    //loop folders
    transientSettings.loopFoldersEnabled = settings.value("loopfoldersenabled", true).toBool();
    ui->loopFoldersCheckbox->setChecked(transientSettings.loopFoldersEnabled);

    //preloading mode
    transientSettings.preloadingMode = settings.value("preloadingmode", 1).toInt();
    ui->preloadingComboBox->setCurrentIndex(transientSettings.preloadingMode);

    //sort mode
    transientSettings.sortMode = settings.value("sortmode", 0).toInt();
    ui->sortComboBox->setCurrentIndex(transientSettings.sortMode);

    //ascending sort
    transientSettings.sortAscending = settings.value("sortascending", true).toBool();
    if (transientSettings.sortAscending)
        ui->ascendingRadioButton0->setChecked(true);
    else
        ui->ascendingRadioButton1->setChecked(true);

    //save recents
    transientSettings.saveRecents = settings.value("saverecents", true).toBool();
    ui->saveRecentsCheckbox->setChecked(transientSettings.saveRecents);

    transientSettings.cursorZoom = settings.value("cursorzoom", true).toBool();
    ui->cursorZoomCheckbox->setChecked(transientSettings.cursorZoom);
}

void QVOptionsDialog::loadShortcuts(bool defaults)
{
    qvApp->getActionManager()->updateShortcuts();

    auto shortcutsList = qvApp->getActionManager()->getShortcutsList();
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
    auto shortcutsList = qvApp->getActionManager()->getShortcutsList();

    QHashIterator<int, QStringList> iter(transientShortcuts);
    while (iter.hasNext())
    {
        iter.next();
        ui->shortcutsTable->item(iter.key(), 1)->setText(ActionManager::stringListToReadableString(iter.value()));
    }
}

void QVOptionsDialog::updateBgColorButton()
{
    QPixmap newPixmap = QPixmap(32, 32);
    newPixmap.fill(transientSettings.bgColor);
    ui->bgColorButton->setIcon(QIcon(newPixmap));
}

void QVOptionsDialog::on_bgColorButton_clicked()
{
    QColor selectedColor = QColorDialog::getColor(transientSettings.bgColor, this);

    if (!selectedColor.isValid())
        return;

    transientSettings.bgColor = selectedColor.name();
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
    if (arg1 > 0)
    {
        transientSettings.bgColorEnabled = true;
        ui->bgColorButton->setEnabled(true);
    }
    else
    {
        transientSettings.bgColorEnabled = false;
        ui->bgColorButton->setEnabled(false);
    }
    updateBgColorButton();
}

void QVOptionsDialog::on_filteringCheckbox_stateChanged(int arg1)
{
    if (arg1 > 0)
    {
        transientSettings.filteringEnabled = true;
    }
    else
    {
        transientSettings.filteringEnabled = false;
    }
}

void QVOptionsDialog::on_scalingCheckbox_stateChanged(int arg1)
{
    if (arg1 > 0)
    {
        transientSettings.scalingEnabled = true;
        ui->scalingTwoCheckbox->setEnabled(true);
    }
    else
    {
        transientSettings.scalingEnabled = false;
        ui->scalingTwoCheckbox->setEnabled(false);
    }
}

void QVOptionsDialog::on_menubarCheckbox_stateChanged(int arg1)
{
    if (arg1 > 0)
    {
        transientSettings.menubarEnabled = true;
    }
    else
    {
        transientSettings.menubarEnabled = false;
    }
}

void QVOptionsDialog::on_cropModeComboBox_currentIndexChanged(int index)
{
    transientSettings.cropMode = index;
}

void QVOptionsDialog::on_slideshowTimerSpinBox_valueChanged(double arg1)
{
    transientSettings.slideshowTimer = arg1;
}

void QVOptionsDialog::on_slideshowDirectionComboBox_currentIndexChanged(int index)
{
    transientSettings.slideshowDirection = index;
}

void QVOptionsDialog::on_scaleFactorSpinBox_valueChanged(int arg1)
{
    transientSettings.scaleFactor = arg1;
}

void QVOptionsDialog::on_scalingTwoCheckbox_stateChanged(int arg1)
{
    if (arg1 > 0)
    {
        transientSettings.scalingTwoEnabled = true;
    }
    else
    {
        transientSettings.scalingTwoEnabled = false;
    }
}

void QVOptionsDialog::on_pastActualSizeCheckbox_stateChanged(int arg1)
{
    if (arg1 > 0)
    {
        transientSettings.pastActualSizeEnabled = true;
    }
    else
    {
        transientSettings.pastActualSizeEnabled = false;
    }
}

void QVOptionsDialog::on_scrollZoomsCheckbox_stateChanged(int arg1)
{
    if (arg1 > 0)
    {
        transientSettings.scrollZoomsEnabled = true;
    }
    else
    {
        transientSettings.scrollZoomsEnabled = false;
    }
}

void QVOptionsDialog::on_titlebarRadioButton0_clicked()
{
    transientSettings.titlebarMode = 0;
}

void QVOptionsDialog::on_titlebarRadioButton1_clicked()
{
    transientSettings.titlebarMode = 1;
}

void QVOptionsDialog::on_titlebarRadioButton2_clicked()
{
    transientSettings.titlebarMode = 2;
}

void QVOptionsDialog::on_windowResizeComboBox_currentIndexChanged(int index)
{
    transientSettings.windowResizeMode = index;
    if (index == 0)
    {
        ui->maxWindowResizeLabel->setEnabled(false);
        ui->maxWindowResizeSpinBox->setEnabled(false);
    }
    else
    {
        ui->maxWindowResizeLabel->setEnabled(true);
        ui->maxWindowResizeSpinBox->setEnabled(true);
    }
}

void QVOptionsDialog::on_maxWindowResizeSpinBox_valueChanged(int arg1)
{
    transientSettings.maxWindowResizedPercentage = arg1;
}

void QVOptionsDialog::on_loopFoldersCheckbox_stateChanged(int arg1)
{
    if (arg1 > 0)
    {
        transientSettings.loopFoldersEnabled = true;
    }
    else
    {
        transientSettings.loopFoldersEnabled = false;
    }
}

void QVOptionsDialog::on_preloadingComboBox_currentIndexChanged(int index)
{
    transientSettings.preloadingMode = index;
}

void QVOptionsDialog::on_shortcutsTable_cellDoubleClicked(int row, int column)
{
    Q_UNUSED(column)
    auto shortcutDialog = new QVShortcutDialog(row);
    shortcutDialog->open();
    connect(shortcutDialog, &QVShortcutDialog::shortcutsListChanged, [this](int index, QStringList stringListShortcuts){
        transientShortcuts.insert(index, stringListShortcuts);
        updateShortcutsTable();
    });
}

void QVOptionsDialog::on_sortComboBox_currentIndexChanged(int index)
{
    transientSettings.sortMode = index;
}

void QVOptionsDialog::on_ascendingRadioButton0_clicked()
{
    transientSettings.sortAscending = true;
}

void QVOptionsDialog::on_ascendingRadioButton1_clicked()
{
    transientSettings.sortAscending = false;
}

void QVOptionsDialog::on_saveRecentsCheckbox_stateChanged(int arg1)
{
    if (arg1 > 0)
    {
        transientSettings.saveRecents = true;
    }
    else
    {
        transientSettings.saveRecents = false;
    }
}

void QVOptionsDialog::on_cursorZoomCheckbox_stateChanged(int arg1)
{
    if (arg1 > 0)
    {
        transientSettings.cursorZoom = true;
    }
    else
    {
        transientSettings.cursorZoom = false;
    }
}
