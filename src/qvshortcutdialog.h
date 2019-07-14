#ifndef QVSHORTCUTDIALOG_H
#define QVSHORTCUTDIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class QVShortcutDialog;
}

class QVShortcutDialog : public QDialog
{
    Q_OBJECT

public:
    struct SShortcut {
        int position;
        QString readableName;
        QString name;
        QStringList defaultShortcuts;
        QStringList shortcuts;
    };

    explicit QVShortcutDialog(SShortcut shortcut, QWidget *parent = nullptr);
    ~QVShortcutDialog() override;

signals:
    void newShortcutObject(SShortcut shortcut);

private slots:
    void on_addButton_clicked();

    void on_subtractButton_clicked();

    void on_keySequenceEdit_editingFinished();

    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::QVShortcutDialog *ui;

    SShortcut shortcut_obj;
};

#endif // QVSHORTCUTDIALOG_H
