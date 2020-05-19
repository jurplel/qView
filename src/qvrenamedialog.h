#ifndef QVRENAMEDIALOG_H
#define QVRENAMEDIALOG_H

#include <QDialog>

namespace Ui {
class QVRenameDialog;
}

class QVRenameDialog : public QDialog
{
    Q_OBJECT
public:
    QVRenameDialog(QWidget *parent = nullptr);
    ~QVRenameDialog() override;

    void setCurrentName(const QString& currentName);
    QString getNewName() const;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::QVRenameDialog *ui;
};

#endif // QVRENAMEDIALOG_H
