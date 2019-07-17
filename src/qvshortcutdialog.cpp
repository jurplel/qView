#include "qvshortcutdialog.h"
#include "ui_qvshortcutdialog.h"

QVShortcutDialog::QVShortcutDialog(const SShortcut &shortcut_object, int i, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVShortcutDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    shortcut_obj = shortcut_object;
    index = i;

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
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
    {
        QStringList newShortcutsList;
        for(int i = 0; i < ui->shortcutsList->count(); i++)
        {
            QListWidgetItem* item = ui->shortcutsList->item(i);
            newShortcutsList << item->text();
        }
        shortcut_obj.shortcuts = newShortcutsList;

        emit newShortcut(shortcut_obj, index);
    }
    else if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole)
    {
        ui->shortcutsList->clear();
        foreach (QString shortcut, shortcut_obj.defaultShortcuts)
        {
            ui->shortcutsList->addItem(shortcut);
        }
    }
}
