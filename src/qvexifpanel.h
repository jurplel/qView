#ifndef QVEXIFPANEL_H
#define QVEXIFPANEL_H

#include <QWidget>

class QLabel;
class QTreeWidget;

class QVExifPanel : public QWidget
{
    Q_OBJECT

public:
    explicit QVExifPanel(QWidget *parent = nullptr);

    void setFilePath(const QString &filePath);

private:
    QLabel *statusLabel;
    QTreeWidget *treeWidget;
};

#endif // QVEXIFPANEL_H
