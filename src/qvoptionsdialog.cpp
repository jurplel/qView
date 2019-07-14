#include "qvoptionsdialog.h"
#include "qvshortcutdialog.h"
#include "ui_qvoptionsdialog.h"
#include <QColorDialog>
#include <QPalette>
#include <QScreen>

static QStringList keyBindingsToStringList(QKeySequence::StandardKey sequence)
{
    auto seqList = QKeySequence::keyBindings(sequence);
    QStringList strings;
    foreach (QKeySequence seq, seqList)
    {
        strings << seq.toString();
    }
    return strings;
}

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


    //shortcuts
    if (!defaults)
        settings.beginGroup("shortcuts");
    else
        settings.beginGroup("emptygroup");

    auto i = new QTableWidgetItem();

    transientShortcuts.open = settings.value("open", keyBindingsToStringList(QKeySequence::Open)).value<QStringList>();
    i->setText(transientShortcuts.open.join(", "));
    ui->shortcutsTable->setItem(0, 1, i->clone());

    transientShortcuts.firstFile = settings.value("firstfile", QKeySequence(Qt::Key_Home).toString()).value<QStringList>();
    i->setText(transientShortcuts.firstFile.join(", "));
    ui->shortcutsTable->setItem(1, 1, i->clone());

    transientShortcuts.previousFile = settings.value("previousfile", QKeySequence(Qt::Key_Left).toString()).value<QStringList>();
    i->setText(transientShortcuts.previousFile.join(", "));
    ui->shortcutsTable->setItem(2, 1, i->clone());

    transientShortcuts.nextFile = settings.value("nextfile", QKeySequence(Qt::Key_Right).toString()).value<QStringList>();
    i->setText(transientShortcuts.nextFile.join(", "));
    ui->shortcutsTable->setItem(3, 1, i->clone());

    transientShortcuts.lastFile = settings.value("lastfile", QKeySequence(Qt::Key_End).toString()).value<QStringList>();
    i->setText(transientShortcuts.lastFile.join(", "));
    ui->shortcutsTable->setItem(4, 1, i->clone());

    transientShortcuts.copy = settings.value("copy", keyBindingsToStringList(QKeySequence::Copy)).value<QStringList>();
    i->setText(transientShortcuts.copy.join(", "));
    ui->shortcutsTable->setItem(5, 1, i->clone());

    transientShortcuts.paste = settings.value("paste", keyBindingsToStringList(QKeySequence::Paste)).value<QStringList>();
    i->setText(transientShortcuts.paste.join(", "));
    ui->shortcutsTable->setItem(6, 1, i->clone());

    transientShortcuts.rotateRight = settings.value("rotateright", QKeySequence(Qt::Key_Up).toString()).value<QStringList>();
    i->setText(transientShortcuts.rotateRight.join(", "));
    ui->shortcutsTable->setItem(7, 1, i->clone());

    transientShortcuts.rotateLeft = settings.value("rotateleft", QKeySequence(Qt::Key_Down).toString()).value<QStringList>();
    i->setText(transientShortcuts.rotateLeft.join(", "));
    ui->shortcutsTable->setItem(8, 1, i->clone());

    transientShortcuts.zoomIn = settings.value("zoomin", keyBindingsToStringList(QKeySequence::ZoomIn)).value<QStringList>();
    i->setText(transientShortcuts.zoomIn.join(", "));
    ui->shortcutsTable->setItem(9, 1, i->clone());

    transientShortcuts.zoomOut = settings.value("zoomout", keyBindingsToStringList(QKeySequence::ZoomOut)).value<QStringList>();
    i->setText(transientShortcuts.zoomOut.join(", "));
    ui->shortcutsTable->setItem(10, 1, i->clone());

    transientShortcuts.resetZoom = settings.value("resetzoom", QKeySequence(Qt::CTRL + Qt::Key_0).toString()).value<QStringList>();
    i->setText(transientShortcuts.resetZoom.join(", "));
    ui->shortcutsTable->setItem(11, 1, i->clone());

    transientShortcuts.mirror = settings.value("mirror", QKeySequence(Qt::Key_F).toString()).value<QStringList>();
    i->setText(transientShortcuts.mirror.join(", "));
    ui->shortcutsTable->setItem(12, 1, i->clone());

    transientShortcuts.flip = settings.value("flip", QKeySequence(Qt::CTRL + Qt::Key_F).toString()).value<QStringList>();
    i->setText(transientShortcuts.flip.join(", "));
    ui->shortcutsTable->setItem(13, 1, i->clone());

    transientShortcuts.fullScreen = settings.value("fullscreen", keyBindingsToStringList(QKeySequence::FullScreen)).value<QStringList>();
    i->setText(transientShortcuts.fullScreen.join(", "));
    ui->shortcutsTable->setItem(14, 1, i->clone());

    transientShortcuts.originalSize = settings.value("originalsize", QKeySequence(Qt::Key_O).toString()).value<QStringList>();
    i->setText(transientShortcuts.originalSize.join(", "));
    ui->shortcutsTable->setItem(15, 1, i->clone());

    transientShortcuts.newWindow = settings.value("newwindow", keyBindingsToStringList(QKeySequence::New)).value<QStringList>();
    i->setText(transientShortcuts.newWindow.join(", "));
    ui->shortcutsTable->setItem(16, 1, i->clone());

    transientShortcuts.nextFrame = settings.value("nextframe", QKeySequence(Qt::Key_N).toString()).value<QStringList>();
    i->setText(transientShortcuts.nextFrame.join(", "));
    ui->shortcutsTable->setItem(17, 1, i->clone());

    transientShortcuts.pause = settings.value("pause", QKeySequence(Qt::Key_P).toString()).value<QStringList>();
    i->setText(transientShortcuts.pause.join(", "));
    ui->shortcutsTable->setItem(18, 1, i->clone());

    transientShortcuts.increaseSpeed = settings.value("increasespeed", QKeySequence(Qt::Key_BracketRight).toString()).value<QStringList>();
    i->setText(transientShortcuts.increaseSpeed.join(", "));
    ui->shortcutsTable->setItem(19, 1, i->clone());

    transientShortcuts.decreaseSpeed = settings.value("decreasespeed", QKeySequence(Qt::Key_BracketLeft).toString()).value<QStringList>();
    i->setText(transientShortcuts.decreaseSpeed.join(", "));
    ui->shortcutsTable->setItem(20, 1, i->clone());

    transientShortcuts.resetSpeed = settings.value("resetspeed", QKeySequence(Qt::Key_Backslash).toString()).value<QStringList>();
    i->setText(transientShortcuts.resetSpeed.join(", "));
    ui->shortcutsTable->setItem(21, 1, i->clone());

    transientShortcuts.showFileInfo = settings.value("showfileinfo", QKeySequence(Qt::Key_I).toString()).value<QStringList>();
    i->setText(transientShortcuts.showFileInfo.join(", "));
    ui->shortcutsTable->setItem(22, 1, i->clone());

    transientShortcuts.options = settings.value("options", keyBindingsToStringList(QKeySequence::Preferences)).value<QStringList>();
    i->setText(transientShortcuts.options.join(", "));
    ui->shortcutsTable->setItem(23, 1, i->clone());

    transientShortcuts.quit = settings.value("quit", keyBindingsToStringList(QKeySequence::Quit)).value<QStringList>();
    i->setText(transientShortcuts.quit.join(", "));
    ui->shortcutsTable->setItem(24, 1, i->clone());
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
        QSettings settings;
        loadSettings(true);
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
    auto shortcutDialog = new QVShortcutDialog();
    shortcutDialog->open();
}

