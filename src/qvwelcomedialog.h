#ifndef QVWELCOMEDIALOG_H
#define QVWELCOMEDIALOG_H

#include <QDialog>

namespace Ui {
class QVWelcomeDialog;
}

class QVWelcomeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QVWelcomeDialog(QWidget *parent = 0);
    ~QVWelcomeDialog();

private slots:
    void on_pushButton_2_clicked();

private:
    Ui::QVWelcomeDialog *ui;
};

#endif // QVWELCOMEDIALOG_H
