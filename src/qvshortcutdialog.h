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
        QString readableName;
        QString name;
        QStringList defaultShortcuts;
        QStringList shortcuts;
    };

    explicit QVShortcutDialog(const SShortcut &shortcut, int i, QWidget *parent = nullptr);
    ~QVShortcutDialog() override;

signals:
    void newShortcut(SShortcut shortcut, int index);

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::QVShortcutDialog *ui;

    SShortcut shortcut_obj;
    int index;
};

#endif // QVSHORTCUTDIALOG_H
