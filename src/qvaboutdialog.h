#ifndef QVABOUTDIALOG_H
#define QVABOUTDIALOG_H

#include <QDialog>

namespace Ui {
class QVAboutDialog;
}

class QVAboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QVAboutDialog(QWidget *parent = 0);
    ~QVAboutDialog();

private:
    Ui::QVAboutDialog *ui;
};

#endif // QVABOUTDIALOG_H
