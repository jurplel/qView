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

    void updateText();

    double getLatestVersionNum() const;
    void setLatestVersionNum(double value);

private:
    Ui::QVAboutDialog *ui;

    double latestVersionNum;
};

#endif // QVABOUTDIALOG_H
