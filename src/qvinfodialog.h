#ifndef QVINFODIALOG_H
#define QVINFODIALOG_H

#include <QDialog>
#include <QFileInfo>

namespace Ui {
class QVInfoDialog;
}

class QVInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QVInfoDialog(QWidget *parent = nullptr);
    ~QVInfoDialog();

    void setInfo(const QFileInfo &value, const int &value2, const int &value3, const int &value4);

    void updateInfo();

private slots:
    void on_actionRefresh_triggered();

private:
    Ui::QVInfoDialog *ui;

    QFileInfo selectedFileInfo;
    int width;
    int height;

    int frameCount;
};

#endif // QVINFODIALOG_H
