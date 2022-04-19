#include "qvgraphicspixmapitem.h"
#include <QPainter>

void QVGraphicsPixmapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    const auto w = pixmap().width();
    const auto h = pixmap().height();
    const auto gridSize = qMax(w / 32, 8);
    static const QColor checkerBoardColors[] = {QColor(150, 150, 150), QColor(200, 200, 200)};
    auto colorIdx = 0;
    auto wCursor = 0;
    auto hCursor = 0;

    for (int i = 0; i < h; i += gridSize, ++hCursor) {
        wCursor = 0;
        for (int j = 0; j < w; j += gridSize, ++wCursor) {
            colorIdx = (hCursor + wCursor) % 2;
            painter->fillRect(j, i,
                              qMin(gridSize, w - j),
                              qMin(gridSize, h - i),
                              checkerBoardColors[colorIdx]);
        }
    }

    QGraphicsPixmapItem::paint(painter, option, widget);
}
