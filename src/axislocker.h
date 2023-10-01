#ifndef AXISLOCKER_H
#define AXISLOCKER_H

#include <QElapsedTimer>
#include <QPoint>
#include <QVariant>

class AxisLocker
{
public:
    AxisLocker();

    QPoint filterMovement(const QPoint delta, const Qt::ScrollPhase phase, const bool isUniAxis);

    void reset();

    void setCustomData(const QVariant &data);

    QVariant getCustomData() const;

private:
    QElapsedTimer lastEvent;
    QPoint swallowedDelta;
    bool horizontalLock;
    bool verticalLock;
    QVariant customData;

    int lockMovementDistance {2};
    int autoResetDuration {250};
};

#endif // AXISLOCKER_H
