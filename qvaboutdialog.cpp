#include "qvaboutdialog.h"
#include "ui_qvaboutdialog.h"

QVAboutDialog::QVAboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVAboutDialog)
{
    ui->setupUi(this);
}

QVAboutDialog::~QVAboutDialog()
{
    delete ui;
}
