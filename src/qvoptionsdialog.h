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

    explicit QVOptionsDialog(QWidget *parent = nullptr);
    ~QVOptionsDialog() override;

    void updateBgColorButton();

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

    void on_slideshowTimerSpinBox_valueChanged(double arg1);

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

    void on_saveRecentsCheckbox_stateChanged(int arg1);

    void on_cursorZoomCheckbox_stateChanged(int arg1);

protected:
    virtual void showEvent(QShowEvent *event) override;

private:
    Ui::QVOptionsDialog *ui;
    void saveSettings();
    void loadSettings(bool defaults = false);
    void loadShortcuts(bool defaults = false);
    void updateShortcutsTable();

    struct STransientSettings
    {
        QString bgColor;
        bool bgColorEnabled;
        bool filteringEnabled;
        bool scalingEnabled;
        int titlebarMode;
        bool menubarEnabled;
        int cropMode;
        double slideshowTimer;
        bool slideshowReversed;
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
        bool saveRecents;
        bool cursorZoom;
    };

    STransientSettings transientSettings;

    QHash<int, QStringList> transientShortcuts;
};


#endif // QVOPTIONSDIALOG_H
