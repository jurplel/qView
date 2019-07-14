#ifndef QVSHORTCUTDIALOG_H
#define QVSHORTCUTDIALOG_H

#include <QDialog>

namespace Ui {
class QVShortcutDialog;
}

class QVShortcutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QVShortcutDialog(QWidget *parent = nullptr);
    ~QVShortcutDialog() override;

private:
    Ui::QVShortcutDialog *ui;
};

#endif // QVSHORTCUTDIALOG_H
