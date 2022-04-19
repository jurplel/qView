#ifndef QVGRAPHICSPIXMAPITEM_H
#define QVGRAPHICSPIXMAPITEM_H

#include <QGraphicsPixmapItem>

class QVGraphicsPixmapItem : public QGraphicsPixmapItem
{
public:
    explicit QVGraphicsPixmapItem(QGraphicsItem *parent = nullptr)
        : QGraphicsPixmapItem(parent) {}

    explicit QVGraphicsPixmapItem(const QPixmap &pixmap, QGraphicsItem *parent = nullptr)
        : QGraphicsPixmapItem(pixmap, parent) {}

private:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // QVGRAPHICSPIXMAPITEM_H
