#ifndef QVABOUTDIALOG_H
#define QVABOUTDIALOG_H

#include <QDialog>
#include <QtNetwork>

namespace Ui {
class QVAboutDialog;
}

class QVAboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QVAboutDialog(QWidget *parent = nullptr);
    ~QVAboutDialog() override;

    void updateCheckForUpdatesButtonState();

private slots:
    void checkForUpdatesButtonClicked();

private:
    Ui::QVAboutDialog *ui;
};

#endif // QVABOUTDIALOG_H
