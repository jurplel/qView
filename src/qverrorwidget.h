#ifndef QVERRORWIDGET_H
#define QVERRORWIDGET_H

#include <QWidget>

namespace Ui {
class QVErrorWidget;
}

class QVErrorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QVErrorWidget(QWidget *parent = nullptr);
    ~QVErrorWidget();

    void setError(int errorNum, const QString &errorString, const QString &fileName);

public slots:
    void settingsUpdated();

private:
    Ui::QVErrorWidget *ui;

    void updateBackgroundColor();
    void updateTextColor();
};

#endif // QVERRORWIDGET_H
