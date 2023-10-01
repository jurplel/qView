#include "axislocker.h"
#include <QDebug>
#include <QPoint>

AxisLocker::AxisLocker()
{
}

QPoint AxisLocker::filterMovement(const QPoint delta, const Qt::ScrollPhase phase, const bool isUniAxis)
{
    if (!lastEvent.isValid() || (phase != Qt::ScrollMomentum && lastEvent.elapsed() >= autoResetDuration))
    {
        swallowedDelta = {};
        horizontalLock = false;
        verticalLock = false;
        customData = {};
    }

    if (phase != Qt::ScrollMomentum)
    {
        lastEvent.restart();
    }

    if (horizontalLock)
    {
        return {delta.x(), 0};
    }

    if (verticalLock)
    {
        return {0, delta.y()};
    }

    swallowedDelta += delta;

    const int absX = qAbs(swallowedDelta.x());
    const int absY = qAbs(swallowedDelta.y());
    const int threshold = isUniAxis ? 1 : lockMovementDistance;

    if (absX >= threshold && absX > absY)
    {
        horizontalLock = true;
        return {swallowedDelta.x(), 0};
    }

    if (absY >= threshold && absY > absX)
    {
        verticalLock = true;
        return {0, swallowedDelta.y()};
    }

    return {};
}

void AxisLocker::reset()
{
    lastEvent.invalidate();
}

void AxisLocker::setCustomData(const QVariant &data)
{
    customData = data;
}

QVariant AxisLocker::getCustomData() const
{
    return customData;
}
