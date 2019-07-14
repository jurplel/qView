#include "qvshortcutdialog.h"
#include "ui_qvshortcutdialog.h"

QVShortcutDialog::QVShortcutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVShortcutDialog)
{
    ui->setupUi(this);
}

QVShortcutDialog::~QVShortcutDialog()
{
    delete ui;
}
