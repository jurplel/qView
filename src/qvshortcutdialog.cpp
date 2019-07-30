#include "qvshortcutdialog.h"
#include "ui_qvshortcutdialog.h"

#include <QDebug>

QVShortcutDialog::QVShortcutDialog(const SShortcut &shortcut_object, int i, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVShortcutDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    shortcut_obj = shortcut_object;
    index = i;
    ui->keySequenceEdit->setKeySequence(shortcut_obj.shortcuts.join(", "));
}

QVShortcutDialog::~QVShortcutDialog()
{
    delete ui;
}

void QVShortcutDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
    {
        QStringList newShortcutsList = ui->keySequenceEdit->keySequence().toString().split(", ");
        shortcut_obj.shortcuts = newShortcutsList;

        emit newShortcut(shortcut_obj, index);
    }
    else if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole)
    {
        ui->keySequenceEdit->setKeySequence(shortcut_obj.defaultShortcuts.join(", "));
    }
}
