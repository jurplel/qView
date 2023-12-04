#ifndef QVCONTENTWIDGET_H
#define QVCONTENTWIDGET_H

#include "qverrorwidget.h"
#include "qvgraphicsview.h"

#include <QStackedWidget>
#include <QWidget>

class QVContentWidget : public QStackedWidget
{
    Q_OBJECT
public:
    QVContentWidget();

    QVGraphicsView *getGraphicsView() { return graphicsView; }
    QVErrorWidget *getErrorWidget() { return errorWidget; }

protected:
    void dropEvent(QDropEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;

    void dragLeaveEvent(QDragLeaveEvent *event) override;

private slots:
    void fileChanged();

private:
    QVGraphicsView *graphicsView;
    QVErrorWidget *errorWidget;
};

#endif // QVCONTENTWIDGET_H
