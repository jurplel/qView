#include "qvshortcutdialog.h"
#include "ui_qvshortcutdialog.h"

#include <QSettings>

#include <QDebug>

QVShortcutDialog::QVShortcutDialog(SShortcut shortcut_object, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVShortcutDialog)
{
    ui->setupUi(this);

    shortcut_obj = shortcut_object;
    foreach (QString shortcut, shortcut_obj.shortcuts)
    {
        ui->shortcutsList->addItem(shortcut);
    }
}

QVShortcutDialog::~QVShortcutDialog()
{
    delete ui;
}

void QVShortcutDialog::on_addButton_clicked()
{
    if (!ui->keySequenceEdit->keySequence().isEmpty())
        ui->shortcutsList->addItem(ui->keySequenceEdit->keySequence().toString());
}

void QVShortcutDialog::on_subtractButton_clicked()
{
    foreach (QListWidgetItem *item, ui->shortcutsList->selectedItems())
    {
        ui->shortcutsList->takeItem(ui->shortcutsList->row(item));
    }
}

void QVShortcutDialog::on_keySequenceEdit_editingFinished()
{
    int value = ui->keySequenceEdit->keySequence()[0];
    QKeySequence shortcut(value);
    ui->keySequenceEdit->setKeySequence(shortcut);
}

void QVShortcutDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole)
    {
        ui->shortcutsList->clear();
        foreach (QString shortcut, shortcut_obj.defaultShortcuts)
        {
            ui->shortcutsList->addItem(shortcut);
        }
    }
}
