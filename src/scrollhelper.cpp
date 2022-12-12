#include "scrollhelper.h"
#include <QDebug>
#include <QtMath>

ScrollHelper::ScrollHelper(QAbstractScrollArea *parent, GetParametersCallback getParametersCallback) : QObject(parent)
{
    hScrollBar = parent->horizontalScrollBar();
    vScrollBar = parent->verticalScrollBar();
    this->getParametersCallback = getParametersCallback;

    animatedScrollTimer = new QTimer(this);
    animatedScrollTimer->setSingleShot(true);
    animatedScrollTimer->setTimerType(Qt::PreciseTimer);
    animatedScrollTimer->setInterval(10);
    connect(animatedScrollTimer, &QTimer::timeout, this, [this]{ handleAnimatedScroll(); });
}

void ScrollHelper::move(QPointF delta)
{
    Parameters p;
    getParametersCallback(p);
    int hMin, hMax, vMin, vMax;
    calculateScrollRange(p.ContentRect.width(), p.UsableViewportRect.width(), p.ContentRect.left() - p.UsableViewportRect.left(), p.ShouldCenter, hMin, hMax);
    calculateScrollRange(p.ContentRect.height(), p.UsableViewportRect.height(), p.ContentRect.top() - p.UsableViewportRect.top(), p.ShouldCenter, vMin, vMax);
    QPointF scrollLocation = QPointF(hScrollBar->value(), vScrollBar->value()) + lastMoveRoundingError;
    qreal scrollDeltaX = hScrollBar->isRightToLeft() ? -delta.x() : delta.x();
    qreal scrollDeltaY = delta.y();
    if (p.ShouldConstrain)
    {
        scrollDeltaX = calculateScrollDelta(scrollLocation.x(), hMin, hMax, scrollDeltaX);
        scrollDeltaY = calculateScrollDelta(scrollLocation.y(), vMin, vMax, scrollDeltaY);
    }
    scrollLocation += QPointF(scrollDeltaX, scrollDeltaY);
    int scrollValueX = qRound(scrollLocation.x());
    int scrollValueY = qRound(scrollLocation.y());
    lastMoveRoundingError = QPointF(scrollLocation.x() - scrollValueX, scrollLocation.y() - scrollValueY);
    int overscrollDistanceX =
        p.ShouldConstrain && scrollValueX < hMin ? scrollValueX - hMin :
        p.ShouldConstrain && scrollValueX > hMax ? scrollValueX - hMax :
        0;
    int overscrollDistanceY =
        p.ShouldConstrain && scrollValueY < vMin ? scrollValueY - vMin :
        p.ShouldConstrain && scrollValueY > vMax ? scrollValueY - vMax :
        0;
    overscrollDistance = QPoint(overscrollDistanceX, overscrollDistanceY);
    hScrollBar->setValue(scrollValueX);
    vScrollBar->setValue(scrollValueY);
}

void ScrollHelper::constrain()
{
    move(QPointF());
    beginAnimatedScroll(-overscrollDistance);
}

void ScrollHelper::cancelAnimation()
{
    animatedScrollTimer->stop();
}

void ScrollHelper::beginAnimatedScroll(QPoint delta)
{
    if (delta.isNull())
        return;
    animatedScrollTotalDelta = delta;
    animatedScrollAppliedDelta = {};
    animatedScrollElapsed.start();
    animatedScrollTimer->start();
}

void ScrollHelper::handleAnimatedScroll()
{
    auto applyScrollDelta = [this](QPoint delta)
    {
        if (delta.x() != 0)
            hScrollBar->setValue(hScrollBar->value() + delta.x());
        if (delta.y() != 0)
            vScrollBar->setValue(vScrollBar->value() + delta.y());
        animatedScrollAppliedDelta += delta;
    };
    qreal elapsed = animatedScrollElapsed.elapsed();
    if (elapsed >= animatedScrollDuration)
    {
        applyScrollDelta(animatedScrollTotalDelta - animatedScrollAppliedDelta);
    }
    else
    {
        const qreal percent = qPow(1.0 - ((qCos(elapsed / animatedScrollDuration * M_PI) + 1.0) / 2.0), 0.2);
        QPoint intermediateDelta = animatedScrollTotalDelta * percent;
        applyScrollDelta(intermediateDelta - animatedScrollAppliedDelta);
        animatedScrollTimer->start();
    }
}

void ScrollHelper::calculateScrollRange(int contentDimension, int viewportDimension, int offset, bool shouldCenter, int &minValue, int &maxValue)
{
    int overflow = contentDimension - viewportDimension;
    if (overflow >= 0)
    {
        minValue = offset;
        maxValue = overflow + offset;
    }
    else if (shouldCenter)
    {
        minValue = overflow / 2 + offset;
        maxValue = minValue;
    }
    else
    {
        minValue = overflow + offset;
        maxValue = offset;
    }
}

qreal ScrollHelper::calculateScrollDelta(qreal currentValue, int minValue, int maxValue, qreal proposedDelta)
{
    const double overflowScaleFactor = 0.05;
    if (proposedDelta < 0 && currentValue + proposedDelta < minValue)
    {
        return currentValue <= minValue ? proposedDelta * overflowScaleFactor :
            (minValue - currentValue) + ((currentValue + proposedDelta) - minValue) * overflowScaleFactor;
    }
    if (proposedDelta > 0 && currentValue + proposedDelta > maxValue)
    {
        return currentValue >= maxValue ? proposedDelta * overflowScaleFactor :
            (maxValue - currentValue) + ((currentValue + proposedDelta) - maxValue) * overflowScaleFactor;
    }
    return proposedDelta;
}
