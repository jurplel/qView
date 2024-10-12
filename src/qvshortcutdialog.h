#ifndef QVSHORTCUTDIALOG_H
#define QVSHORTCUTDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include "shortcutmanager.h"

namespace Ui {
class QVShortcutDialog;
}

class QVShortcutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QVShortcutDialog(int index, QWidget *parent = nullptr);
    ~QVShortcutDialog() override;

    QString shortcutAlreadyBound(const QKeySequence &chosenSequence, const QString &exemptShortcut);
    void acceptValidated();

signals:
    void shortcutsListChanged(int index, QStringList shortcutsStringList);

private slots:
    void buttonBoxClicked(QAbstractButton *button);

private:
    void done(int r) override;

    Ui::QVShortcutDialog *ui;

    ShortcutManager::SShortcut shortcutObject;
    int index;
};

#endif // QVSHORTCUTDIALOG_H
