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
    explicit QVWelcomeDialog(QWidget *parent = nullptr);
    ~QVWelcomeDialog() override;

private:
    Ui::QVWelcomeDialog *ui;
};

#endif // QVWELCOMEDIALOG_H
