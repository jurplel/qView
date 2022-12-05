#ifndef SCROLLHELPER_H
#define SCROLLHELPER_H

#include <QAbstractScrollArea>
#include <QElapsedTimer>
#include <QScrollBar>
#include <QTimer>

class ScrollHelper : public QObject
{
    Q_OBJECT
public:
    struct Parameters
    {
        QSize ScaledContentSize;
        QRect UsableViewportRect;
        bool IsMirrored;
        bool IsFlipped;
        bool ShouldConstrain;
        bool ShouldCenter;
    };

    typedef std::function<void(Parameters &)> GetParametersCallback;

    explicit ScrollHelper(QAbstractScrollArea *parent, GetParametersCallback getParametersCallback);

    void move(QPointF delta);

    void constrain();

    void cancelAnimation();

private:
    void beginAnimatedScroll(QPoint delta);

    void handleAnimatedScroll();

    static void calculateScrollRange(int contentDimension, int viewportDimension, int offset, bool shouldCenter, int &minValue, int &maxValue);

    static qreal calculateScrollDelta(qreal currentValue, int minValue, int maxValue, qreal proposedDelta);

    QScrollBar *hScrollBar;
    QScrollBar *vScrollBar;
    GetParametersCallback getParametersCallback;
    QPointF lastMoveRoundingError {};
    QPoint overscrollDistance {};

    QTimer *animatedScrollTimer;
    QPoint animatedScrollTotalDelta;
    QPoint animatedScrollAppliedDelta;
    QElapsedTimer animatedScrollElapsed;
    const qreal animatedScrollDuration {250.0};
};

#endif // SCROLLHELPER_H
