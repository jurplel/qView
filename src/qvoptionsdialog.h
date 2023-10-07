#ifndef QVOPTIONSDIALOG_H
#define QVOPTIONSDIALOG_H

#include "qvnamespace.h"
#include "qvshortcutdialog.h"

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
    void done(int r) override;

    void modifySetting(QString key, QVariant value);
    void saveSettings();
    void syncSettings(bool defaults = false, bool makeConnections = false);
    void syncCheckbox(QCheckBox *checkbox, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncRadioButtons(QList<QRadioButton*> buttons, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncComboBox(QComboBox *comboBox, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncComboBoxData(QComboBox *comboBox, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncSpinBox(QSpinBox *spinBox, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncDoubleSpinBox(QDoubleSpinBox *doubleSpinBox, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncLineEdit(QLineEdit *lineEdit, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncShortcuts(bool defaults = false);
    void updateShortcutsTable();
    void updateButtonBox();
    void bgColorButtonClicked();
    void updateBgColorButton();
    void restartNotifyForCheckbox(const QString &key, const int state);
    void populateLanguages();
    void populateComboBoxes();

    const QMap<Qv::AfterDelete, QString> mapAfterDelete();
    const QMap<Qv::AfterMatchingSize, QString> mapAfterMatchingSize();
    const QMap<Qv::CalculatedZoomMode, QString> mapCalculatedZoomMode();
    const QMap<Qv::ColorSpaceConversion, QString> mapColorSpaceConversion();
    const QMap<Qv::PreloadMode, QString> mapPreloadMode();
    const QMap<Qv::SortMode, QString> mapSortMode();
    const QMap<Qv::TitleBarText, QString> mapTitleBarText();
    const QMap<Qv::WindowResizeMode, QString> mapWindowResizeMode();
    const QMap<Qv::ViewportClickAction, QString> mapViewportClickAction();
    const QMap<Qv::ViewportDragAction, QString> mapViewportDragAction();
    const QMap<Qv::ViewportScrollAction, QString> mapViewportScrollAction();

private slots:
    void shortcutCellDoubleClicked(int row, int column);

    void buttonBoxClicked(QAbstractButton *button);

    void bgColorCheckboxStateChanged(int state);

    void scalingCheckboxStateChanged(int state);

    void titlebarComboBoxCurrentIndexChanged(int index);

    void windowResizeComboBoxCurrentIndexChanged(int index);

    void constrainImagePositionCheckboxStateChanged(int state);

    void languageComboBoxCurrentIndexChanged(int index);

    void middleButtonModeChanged();

private:
    Ui::QVOptionsDialog *ui;

    QHash<QString, QVariant> transientSettings;

    QList<QStringList> transientShortcuts;

    bool languageRestartMessageShown {false};
};


#endif // QVOPTIONSDIALOG_H
