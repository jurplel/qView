#ifndef QVOPTIONSDIALOG_H
#define QVOPTIONSDIALOG_H

#include "qvshortcutdialog.h"

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
    static QStringList keyBindingsToStringList(QKeySequence::StandardKey sequence)
    {
        auto seqList = QKeySequence::keyBindings(sequence);
        QStringList strings;
        foreach (QKeySequence seq, seqList)
        {
            strings << seq.toString();
        }
        return strings;
    }

    static QList<QKeySequence> stringListToKeySequenceList(QStringList stringList)
    {

        QList<QKeySequence> keySequences;
        foreach (QString string, stringList)
        {
            keySequences << QKeySequence::fromString(string);
        }
        return keySequences;
    }

    explicit QVOptionsDialog(QWidget *parent = nullptr);
    ~QVOptionsDialog() override;

    void updateBgColorButton();

    const QList<QVShortcutDialog::SShortcut>& getTransientShortcuts() const {return transientShortcuts; }

signals:
    void optionsSaved();

private slots:
    void on_bgColorButton_clicked();

    void on_bgColorCheckbox_stateChanged(int arg1);

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_filteringCheckbox_stateChanged(int arg1);

    void on_scalingCheckbox_stateChanged(int arg1);

    void on_menubarCheckbox_stateChanged(int arg1);

    void on_cropModeComboBox_currentIndexChanged(int index);

    void on_slideshowTimerSpinBox_valueChanged(int arg1);

    void on_slideshowDirectionComboBox_currentIndexChanged(int index);

    void on_scaleFactorSpinBox_valueChanged(int arg1);

    void on_scalingTwoCheckbox_stateChanged(int arg1);

    void on_pastActualSizeCheckbox_stateChanged(int arg1);

    void on_scrollZoomsCheckbox_stateChanged(int arg1);

    void on_titlebarRadioButton0_clicked();

    void on_titlebarRadioButton1_clicked();

    void on_titlebarRadioButton2_clicked();

    void on_windowResizeComboBox_currentIndexChanged(int index);

    void on_maxWindowResizeSpinBox_valueChanged(int arg1);

    void on_loopFoldersCheckbox_stateChanged(int arg1);

    void on_preloadingComboBox_currentIndexChanged(int index);

    void on_shortcutsTable_cellDoubleClicked(int row, int column);

    void on_sortComboBox_currentIndexChanged(int index);

    void on_ascendingRadioButton0_clicked();

    void on_ascendingRadioButton1_clicked();

protected:
    virtual void showEvent(QShowEvent *event) override;

private:
    Ui::QVOptionsDialog *ui;
    void saveSettings();
    void loadSettings(const bool defaults = false);
    void loadShortcuts(const bool defaults = false);
    void updateShortcuts();

    struct STransientSettings
    {
        QString bgColor;
        bool bgColorEnabled;
        bool filteringEnabled;
        bool scalingEnabled;
        int titlebarMode;
        bool menubarEnabled;
        int cropMode;
        int slideshowTimer;
        int slideshowDirection;
        int scaleFactor;
        bool resizeScaleEnabled;
        bool scalingTwoEnabled;
        bool pastActualSizeEnabled;
        bool scrollZoomsEnabled;
        int windowResizeMode;
        int maxWindowResizedPercentage;
        bool loopFoldersEnabled;
        int preloadingMode;
        int sortMode;
        bool sortAscending;
    };

    STransientSettings transientSettings;

    QList<QVShortcutDialog::SShortcut> transientShortcuts;
};


#endif // QVOPTIONSDIALOG_H
