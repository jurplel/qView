#ifndef QVOPTIONSDIALOG_H
#define QVOPTIONSDIALOG_H

#include <QDialog>

namespace Ui {
class QVOptionsDialog;
}

class QVOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QVOptionsDialog(QWidget *parent = 0);
    ~QVOptionsDialog();

private:
    Ui::QVOptionsDialog *ui;
};

#endif // QVOPTIONSDIALOG_H
