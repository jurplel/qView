#include "qvinfodialog.h"
#include "ui_qvinfodialog.h"
#include <QDateTime>
#include <QLocale>
#include <QMimeDatabase>

QVInfoDialog::QVInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVInfoDialog)
{
    ui->setupUi(this);
    ui->scrollAreaWidgetContents->addAction(ui->actionRefresh);
}

QVInfoDialog::~QVInfoDialog()
{
    delete ui;
}

void QVInfoDialog::setInfo(const QFileInfo &value, const int &value2, const int &value3)
{
    selectedFileInfo = value;
    width = value2;
    height = value3;
    updateInfo();
}

void QVInfoDialog::on_actionRefresh_triggered()
{
    updateInfo();
}

void QVInfoDialog::updateInfo()
{
    QLocale locale = QLocale::system();
    QMimeDatabase mimedb;
    QMimeType mime = mimedb.mimeTypeForFile(selectedFileInfo.filePath(), QMimeDatabase::MatchContent);
    const float megapixels = float(int(((float)(width*height))/1000000 * 10 + 0.5)) / 10 ;

    ui->nameLabel->setText(selectedFileInfo.fileName());
    ui->typeLabel->setText(mime.name());
    ui->locationLabel->setText(selectedFileInfo.path());
    ui->sizeLabel->setText(locale.formattedDataSize(selectedFileInfo.size()) + " (" + locale.toString(selectedFileInfo.size()) + " bytes)");
    ui->modifiedLabel->setText(selectedFileInfo.lastModified().toString(locale.dateTimeFormat()));
    ui->dimensionsLabel->setText(QString::number(width) + " x " + QString::number(height) + " (" + QString::number(megapixels) + "MP)");
}
