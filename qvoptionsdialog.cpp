#include "qvoptionsdialog.h"
#include "ui_qvoptionsdialog.h"

QVOptionsDialog::QVOptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVOptionsDialog)
{
    ui->setupUi(this);
}

QVOptionsDialog::~QVOptionsDialog()
{
    delete ui;
}
