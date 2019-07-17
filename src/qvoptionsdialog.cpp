#include "qvoptionsdialog.h"
#include "ui_qvoptionsdialog.h"
#include <QColorDialog>
#include <QPalette>
#include <QScreen>

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

    // Set default shortcuts
    transientShortcuts.append({"Open", "open", keyBindingsToStringList(QKeySequence::Open), {}});
    transientShortcuts.append({"First File", "firstfile", QStringList(QKeySequence(Qt::Key_Home).toString()), {}});
    transientShortcuts.append({"Previous File", "previousfile", QStringList(QKeySequence(Qt::Key_Left).toString()), {}});
    transientShortcuts.append({"Next File", "nextfile", QStringList(QKeySequence(Qt::Key_Right).toString()), {}});
    transientShortcuts.append({"Last File", "lastfile", QStringList(QKeySequence(Qt::Key_End).toString()), {}});
    transientShortcuts.append({"Copy", "copy", keyBindingsToStringList(QKeySequence::Copy), {}});
    transientShortcuts.append({"Paste", "paste", keyBindingsToStringList(QKeySequence::Paste), {}});
    transientShortcuts.append({"Rotate Right", "rotateright", QStringList(QKeySequence(Qt::Key_Up).toString()), {}});
    transientShortcuts.append({"Rotate Left", "rotateleft", QStringList(QKeySequence(Qt::Key_Down).toString()), {}});
    transientShortcuts.append({"Zoom In", "zoomin", keyBindingsToStringList(QKeySequence::ZoomIn), {}});
    transientShortcuts.append({"Zoom Out", "zoomout", keyBindingsToStringList(QKeySequence::ZoomOut), {}});
    transientShortcuts.append({"Reset Zoom", "resetzoom", QStringList(QKeySequence(Qt::CTRL + Qt::Key_0).toString()), {}});
    transientShortcuts.append({"Mirror", "mirror", QStringList(QKeySequence(Qt::Key_F).toString()), {}});
    transientShortcuts.append({"Flip", "flip", QStringList(QKeySequence(Qt::CTRL + Qt::Key_F).toString()), {}});
    transientShortcuts.append({"Full Screen", "fullscreen", keyBindingsToStringList(QKeySequence::FullScreen), {}});
    //Fixes alt+enter only working with numpad enter when using qt's standard keybinds
    #ifdef Q_OS_WIN
        transientShortcuts.replace(14, {"Full Screen", "fullscreen", keyBindingsToStringList(QKeySequence::FullScreen) << QKeySequence(Qt::ALT + Qt::Key_Return).toString(), {}});
    #endif
    transientShortcuts.append({"Original Size", "originalsize", QStringList(QKeySequence(Qt::Key_O).toString()), {}});
    transientShortcuts.append({"New Window", "newwindow", keyBindingsToStringList(QKeySequence::New), {}});
    transientShortcuts.append({"Next Frame", "nextframe", QStringList(QKeySequence(Qt::Key_N).toString()), {}});
    transientShortcuts.append({"Pause", "pause", QStringList(QKeySequence(Qt::Key_P).toString()), {}});
    transientShortcuts.append({"Increase Speed", "increasespeed", QStringList(QKeySequence(Qt::Key_BracketRight).toString()), {}});
    transientShortcuts.append({"Decrease Speed", "decreasespeed", QStringList(QKeySequence(Qt::Key_BracketLeft).toString()), {}});
    transientShortcuts.append({"Reset Speed", "resetspeed", QStringList(QKeySequence(Qt::Key_Backslash).toString()), {}});
    transientShortcuts.append({"Show File Info", "showfileinfo", QStringList(QKeySequence(Qt::Key_I).toString()), {}});
    transientShortcuts.append({"Options", "options", keyBindingsToStringList(QKeySequence::Preferences), {}});
    transientShortcuts.append({"Quit", "quit", keyBindingsToStringList(QKeySequence::Quit), {}});
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

    // Shortcuts are saved here too for now
    settings.endGroup();
    settings.beginGroup("shortcuts");

    QListIterator<QVShortcutDialog::SShortcut> iter(transientShortcuts);
    while (iter.hasNext())
    {
        auto value = iter.next();
        settings.setValue(value.name, value.shortcuts);
    }

    emit optionsSaved();
}

void QVOptionsDialog::loadSettings(const bool defaults)
{
    QSettings settings;
    if (!defaults)
        settings.beginGroup("options");
    else
        settings.beginGroup("emptygroup");

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
    transientSettings.slideshowTimer = settings.value("slideshowtimer", 5).toInt();
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
}

void QVOptionsDialog::loadShortcuts(const bool defaults)
{
    QSettings settings;
    if (!defaults)
        settings.beginGroup("shortcuts");
    else
        settings.beginGroup("emptygroup");

    // Read saved custom shortcuts
    QMutableListIterator<QVShortcutDialog::SShortcut> iter(transientShortcuts);
    while (iter.hasNext())
    {
        auto value = iter.next();
        iter.setValue({value.readableName, value.name, value.defaultShortcuts, settings.value(value.name, value.defaultShortcuts).value<QStringList>()});
    }

    updateShortcuts();
}

void QVOptionsDialog::updateShortcuts() {
    ui->shortcutsTable->setRowCount(transientShortcuts.length());

    auto item = new QTableWidgetItem();

    int i = 0;
    QListIterator<QVShortcutDialog::SShortcut> iter(transientShortcuts);
    while (iter.hasNext())
    {
        auto value = iter.next();

        item->setText(value.readableName);
        ui->shortcutsTable->setItem(i, 0, item->clone());

        item->setText(value.shortcuts.join(", "));
        ui->shortcutsTable->setItem(i, 1, item->clone());
        i++;
    }

    delete item;

    // Hide next window keybind on platforms other than macOS (cause it doesn't work)
    #ifndef Q_OS_MACX
    ui->shortcutsTable->hideRow(16);
    #endif
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

void QVOptionsDialog::on_slideshowTimerSpinBox_valueChanged(int arg1)
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
    auto shortcutDialog = new QVShortcutDialog(transientShortcuts.value(row), row);
    shortcutDialog->open();
    connect(shortcutDialog, &QVShortcutDialog::newShortcut, [this](QVShortcutDialog::SShortcut shortcut, int index){
        transientShortcuts.replace(index, shortcut);
        updateShortcuts();
    });
}

