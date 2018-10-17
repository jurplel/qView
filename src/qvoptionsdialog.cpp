#include "qvoptionsdialog.h"
#include "ui_qvoptionsdialog.h"
#include <QColorDialog>
#include <QPalette>

QVOptionsDialog::QVOptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVOptionsDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
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
    emit optionsSaved();
}

void QVOptionsDialog::loadSettings(const bool defaults)
{
    QSettings settings;
    if (!defaults)
        settings.beginGroup("options");

    //bgcolor
    transientSettings.bgColor = settings.value("bgcolor", QString("#212121")).toString();
    ui->bgColorButton->setText(transientSettings.bgColor);
    updateBgColorButton();

    transientSettings.bgColorEnabled = settings.value("bgcolorenabled", true).toBool();
    if (transientSettings.bgColorEnabled)
    {
        ui->bgColorCheckbox->setChecked(true);
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

    transientSettings.windowResizeMode = settings.value("windowresizemode", 0).toInt();
    ui->windowResizeComboBox->setCurrentIndex(transientSettings.windowResizeMode);
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
}
