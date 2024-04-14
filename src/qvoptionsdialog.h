#ifndef QVOPTIONSDIALOG_H
#define QVOPTIONSDIALOG_H

#include "qvnamespace.h"
#include "qvshortcutdialog.h"

#include <QDialog>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QSpinBox>
#include <QTableWidget>

namespace Ui {
class QVOptionsDialog;

template <typename TEnum>
using ComboBoxItems = QVector<std::pair<TEnum, QString>>;
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
    void syncSpinBox(QSpinBox *spinBox, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncDoubleSpinBox(QDoubleSpinBox *doubleSpinBox, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncLineEdit(QLineEdit *lineEdit, const QString &key, bool defaults = false, bool makeConnection = false);
    void syncShortcuts(bool defaults = false);
    void updateShortcutsTable();
    void syncFormats(bool defaults = false);
    void updateButtonBox();
    void bgColorButtonClicked();
    void updateBgColorButton();
    void restartNotifyForCheckbox(const QString &key, const int state);
    void populateCategories();
    void populateLanguages();
    void populateComboBoxes();

    const Ui::ComboBoxItems<Qv::AfterDelete> mapAfterDelete();
    const Ui::ComboBoxItems<Qv::AfterMatchingSize> mapAfterMatchingSize();
    const Ui::ComboBoxItems<Qv::CalculatedZoomMode> mapCalculatedZoomMode();
    const Ui::ComboBoxItems<Qv::ColorSpaceConversion> mapColorSpaceConversion();
    const Ui::ComboBoxItems<Qv::PreloadMode> mapPreloadMode();
    const Ui::ComboBoxItems<Qv::SlideshowDirection> mapSlideshowDirection();
    const Ui::ComboBoxItems<Qv::SortMode> mapSortMode();
    const Ui::ComboBoxItems<Qv::TitleBarText> mapTitleBarText();
    const Ui::ComboBoxItems<Qv::WindowResizeMode> mapWindowResizeMode();
    const Ui::ComboBoxItems<Qv::ViewportClickAction> mapViewportClickAction();
    const Ui::ComboBoxItems<Qv::ViewportDragAction> mapViewportDragAction();
    const Ui::ComboBoxItems<Qv::ViewportScrollAction> mapViewportScrollAction();

private slots:
    void shortcutCellDoubleClicked(int row, int column);

    void buttonBoxClicked(QAbstractButton *button);

    void bgColorCheckboxStateChanged(int state);

    void scalingCheckboxStateChanged(int state);

    void titlebarComboBoxCurrentIndexChanged(int index);

    void windowResizeComboBoxCurrentIndexChanged(int index);

    void fitZoomLimitCheckboxStateChanged(int state);

    void constrainImagePositionCheckboxStateChanged(int state);

    void languageComboBoxCurrentIndexChanged(int index);

    void formatsItemChanged(QTableWidgetItem *item);

    void middleButtonModeChanged();

private:
    Ui::QVOptionsDialog *ui;

    QHash<QString, QVariant> transientSettings;

    QList<QStringList> transientShortcuts;

    QSet<QString> transientDisabledFileExtensions;

    bool isInitialLoad {true};

    bool languageRestartMessageShown {false};

    bool isLoadingFormats {false};
};


#endif // QVOPTIONSDIALOG_H
