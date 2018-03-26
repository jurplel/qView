#include "qvoptionsdialog.h"
#include "ui_qvoptionsdialog.h"
#include <QColorDialog>
#include <QDebug>
#include <QPalette>

QVOptionsDialog::QVOptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVOptionsDialog)
{
    ui->setupUi(this);
    setWindowFlag(Qt::Tool);
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
    settings.setValue("bgcolor", transientSettings.bgcolor);
    settings.setValue("bgcolorenabled", transientSettings.bgcolorenabled);
    settings.setValue("cursorenabled", transientSettings.cursorenabled);
    emit optionsSaved();
}

void QVOptionsDialog::loadSettings()
{
    QSettings settings;

    //bgcolor
    transientSettings.bgcolor = settings.value("bgcolor", QString("#212121")).toString();
    loadedColor.setNamedColor(transientSettings.bgcolor);
    setBgColorButtonColor(loadedColor);

    transientSettings.bgcolorenabled = settings.value("bgcolorenabled", true).toBool();
    if (transientSettings.bgcolorenabled)
    {
        ui->bgColorCheckbox->setChecked(true);
    }
    else
    {
        ui->bgColorCheckbox->setChecked(false);
        ui->bgColorButton->setEnabled(false);
        ui->bgColorLabel->setEnabled(false);
    }

    //mousecursor
    transientSettings.cursorenabled = settings.value("cursorenabled", true).toBool();
    ui->cursorCheckbox->setChecked(transientSettings.cursorenabled);
}


void QVOptionsDialog::on_bgColorButton_clicked()
{
    QColor selectedColor = QColorDialog::getColor(loadedColor, this);

    if (!selectedColor.isValid())
        return;

    transientSettings.bgcolor = selectedColor.name();
    setBgColorButtonColor(selectedColor);
}

void QVOptionsDialog::setBgColorButtonColor(QColor newColor)
{
    QPalette newPalette = ui->bgColorButton->palette();
    newPalette.setColor(QPalette::Button, newColor);
    ui->bgColorButton->setPalette(newPalette);
    ui->bgColorButton->setText(newColor.name());
}

void QVOptionsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (ui->buttonBox->buttonRole(button) == 8 || ui->buttonBox->buttonRole(button) == 0)
    {
        saveSettings();
    }
}

void QVOptionsDialog::on_bgColorCheckbox_stateChanged(int arg1)
{
    if (arg1 > 0)
    {
        transientSettings.bgcolorenabled = true;
        ui->bgColorButton->setEnabled(true);
        ui->bgColorLabel->setEnabled(true);
    }
    else
    {
        transientSettings.bgcolorenabled = false;
        ui->bgColorButton->setEnabled(false);
        ui->bgColorLabel->setEnabled(false);
    }
}

void QVOptionsDialog::on_cursorCheckbox_stateChanged(int arg1)
{
    if (arg1 > 0)
    {
        transientSettings.cursorenabled = true;
    }
    else
    {
        transientSettings.cursorenabled = false;
    }
}
