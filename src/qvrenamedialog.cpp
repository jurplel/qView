#include "qvrenamedialog.h"
#include "ui_qvrenamedialog.h"
#include <QPushButton>

QVRenameDialog::QVRenameDialog(QWidget *parent):
    QDialog(parent),
    ui(new Ui::QVRenameDialog)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->newNameLE->installEventFilter(this);

    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint | Qt::CustomizeWindowHint));
}

void QVRenameDialog::setCurrentName(const QString& currentName) {
    ui->currentNameLE->setText(currentName);
}

QString QVRenameDialog::getNewName() const
{
    return ui->newNameLE->text();
}

bool QVRenameDialog::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == ui->newNameLE)
    {
        if(event->type() == QEvent::KeyRelease)
        {
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!ui->newNameLE->text().isEmpty());
        }
    }
    return QDialog::eventFilter(obj, event);
}

QVRenameDialog::~QVRenameDialog()
{
    delete ui;
}
