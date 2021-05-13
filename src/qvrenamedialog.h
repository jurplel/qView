#ifndef QVRENAMEDIALOG_H
#define QVRENAMEDIALOG_H

#include <QInputDialog>
#include <QFileInfo>

class QVRenameDialog : public QInputDialog
{
    Q_OBJECT
public:
    QVRenameDialog(QWidget *parent, QFileInfo fileInfo);

    void onFinished(int result);

signals:
    void newFileToOpen(const QString &filePath);

protected:
    void showEvent(QShowEvent *event) override;

private:
    QFileInfo fileInfo;
};

#endif // QVRENAMEDIALOG_H
