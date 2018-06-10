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

signals:
    void optionsSaved();

private slots:
    void on_bgColorButton_clicked();

    void on_bgColorCheckbox_stateChanged(int arg1);

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_filteringCheckbox_stateChanged(int arg1);

    void on_scalingCheckbox_stateChanged(int arg1);

    void on_titlebarModeComboBox_currentIndexChanged(int index);

    void on_menubarCheckbox_stateChanged(int arg1);

    void on_cropModeComboBox_currentIndexChanged(int index);

protected:
    virtual void showEvent(QShowEvent *event);

private:
    Ui::QVOptionsDialog *ui;
    void saveSettings();
    void loadSettings();

    QColor loadedColor;

    struct STransientSettings
    {
        QString bgColor;
        bool bgColorEnabled;
        bool filteringEnabled;
        bool scalingEnabled;
        int titlebarMode;
        bool menubarEnabled;
        int cropMode;
    };

    STransientSettings transientSettings;
};


#endif // QVOPTIONSDIALOG_H
