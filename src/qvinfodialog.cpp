#include "qvinfodialog.h"
#include "ui_qvinfodialog.h"

QVInfoDialog::QVInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVInfoDialog)
{
    ui->setupUi(this);
}

QVInfoDialog::~QVInfoDialog()
{
    delete ui;
}

QFileInfo QVInfoDialog::getSelectedFileInfo() const
{
    return selectedFileInfo;
}

void QVInfoDialog::setSelectedFileInfo(const QFileInfo &value)
{
    selectedFileInfo = value;
}
