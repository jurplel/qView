#ifndef QVOPTIONSDIALOG_H
#define QVOPTIONSDIALOG_H

#include "qvshortcutdialog.h"
#include "settingsmanager.h"

#include <QDialog>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QSpinBox>

namespace Ui {
class QVOptionsDialog;
}

class QVOptionsDialog : public QDialog
{
    Q_OBJECT

public:

    explicit QVOptionsDialog(QWidget *parent = nullptr);
    ~QVOptionsDialog() override;


protected:
    void closeEvent(QCloseEvent *event) override;

    void actuallyClose(QCloseEvent *event);

    void modifySetting(QString key, QVariant value);
    void saveSettings();
    void syncSettings(bool defaults = false, bool makeConnections = false);
    void syncCheckbox(QCheckBox *checkbox, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncRadioButtons(QList<QRadioButton*> buttons, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncComboBox(QComboBox *comboBox, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncSpinBox(QSpinBox *spinBox, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncDoubleSpinBox(QDoubleSpinBox *doubleSpinBox, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncShortcuts(bool defaults = false);
    void updateShortcutsTable();
    void bgColorButtonClicked();
    void updateBgColorButton();

private slots:
    void on_shortcutsTable_cellDoubleClicked(int row, int column);

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_bgColorCheckbox_stateChanged(int arg1);

    void on_scalingCheckbox_stateChanged(int arg1);

    void on_windowResizeComboBox_currentIndexChanged(int index);

private:
    Ui::QVOptionsDialog *ui;

    QHash<QString, QVariant> transientSettings;

    QHash<int, QStringList> transientShortcuts;
};


#endif // QVOPTIONSDIALOG_H
