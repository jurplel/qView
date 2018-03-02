#ifndef QVGRAPHICSVIEW_H
#define QVGRAPHICSVIEW_H

#include <QGraphicsView>

class QVGraphicsView : public QGraphicsView
{

public:
    QVGraphicsView(QWidget *parent = nullptr);

protected:
    virtual void wheelEvent(QWheelEvent *event);
};

#endif // QVGRAPHICSVIEW_H
