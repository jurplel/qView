#ifndef QVOPTIONSDIALOG_H
#define QVOPTIONSDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QAbstractButton>

namespace Ui {
class QVOptionsDialog;
}

class QVOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QVOptionsDialog(QWidget *parent = 0);
    ~QVOptionsDialog();

    void setBgColorButtonColor(QColor newColor);

private slots:
    void on_bgColorButton_clicked();

    void on_bgColorCheckbox_stateChanged(int arg1);

    void on_buttonBox_clicked(QAbstractButton *button);

protected:
    virtual void showEvent(QShowEvent *event);

private:
    Ui::QVOptionsDialog *ui;
    void saveSettings();
    void loadSettings();

    QColor loadedColor;

    struct STransientSettings
    {
        QString bgcolor;
        bool bgcolorenabled;
    };

    STransientSettings transientSettings;
};


#endif // QVOPTIONSDIALOG_H
