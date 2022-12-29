#include "qvrenamedialog.h"

#include <QDir>
#include <QMessageBox>

QVRenameDialog::QVRenameDialog(QWidget *parent, QFileInfo fileInfo) :
    QInputDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    this->fileInfo = fileInfo;

    setWindowTitle(tr("Rename…"));
    setLabelText(tr("Filename:"));
    setTextValue(fileInfo.fileName());
    resize(350, height());

    connect(this, &QInputDialog::finished, this, &QVRenameDialog::onFinished);
}

void QVRenameDialog::onFinished(int result)
{
    if (!fileInfo.isWritable())
    {
        QMessageBox::critical(this, tr("Error"), tr("Could not rename %1:\nGrant write permission or ensure the file is not read-only").arg(fileInfo.fileName()));
        return;
    }

    if (result)
    {
        const auto newFileName = textValue();
        const auto newFilePath = QDir::cleanPath(fileInfo.absolutePath() + QDir::separator() + newFileName);

        emit readyToRenameFile();

        if (fileInfo.absoluteFilePath() != newFilePath)
        {
            if (QFile::rename(fileInfo.absoluteFilePath(), newFilePath))
            {
                emit newFileToOpen(newFilePath);
            }
            else
            {
                QMessageBox::critical(this, tr("Error"), tr("Could not rename %1:\n(Check that all characters are valid)").arg(fileInfo.fileName()));
            }
        }
    }
}

void QVRenameDialog::showEvent(QShowEvent *event)
{
    QInputDialog::showEvent(event);

    QLineEdit *lineEdit = findChild<QLineEdit*>();
    const auto &lastDot = lineEdit->text().lastIndexOf(".");
    if (lastDot != -1)
        lineEdit->setSelection(0, lastDot);
}
