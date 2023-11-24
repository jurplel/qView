#ifndef QVNAMESPACE_H
#define QVNAMESPACE_H

#include <QSet>
#include <QString>

namespace Qv
{
    inline constexpr int SessionStateVersion = 1;

    enum class AfterDelete
    {
        MoveBack = 0,
        DoNothing = 1,
        MoveForward = 2
    };

    enum class AfterMatchingSize
    {
        AvoidRepositioning = 0,
        CenterOnPrevious = 1,
        CenterOnScreen = 2
    };

    enum class CalculatedZoomMode
    {
        ZoomToFit = 0,
        FillWindow = 1
    };

    enum class ClickOrDrag
    {
        Click = 0,
        Drag = 1
    };

    enum class ColorSpaceConversion
    {
        Disabled = 0,
        AutoDetect = 1,
        SRgb = 2,
        DisplayP3 = 3
    };

    enum class PreloadMode
    {
        Disabled = 0,
        Adjacent = 1,
        Extended = 2
    };

    enum class SlideshowDirection
    {
        Forward = 0,
        Backward = 1,
        Random = 2
    };

    enum class SortMode
    {
        Name = 0,
        DateModified = 1,
        DateCreated = 2,
        Size = 3,
        Type = 4,
        Random = 5
    };

    enum class TitleBarText
    {
        Basic = 0,
        Minimal = 1,
        Practical = 2,
        Verbose = 3,
        Custom = 4
    };

    enum class ViewportClickAction
    {
        None = 0,
        ZoomToFit = 1,
        FillWindow = 2,
        OriginalSize = 3,
        ToggleFullScreen = 4,
        ToggleTitlebarHidden = 5
    };

    enum class ViewportDragAction
    {
        None = 0,
        Pan = 1,
        MoveWindow = 2
    };

    enum class ViewportScrollAction
    {
        None = 0,
        Zoom = 1,
        Navigate = 2,
        Pan = 3
    };

    enum class WindowResizeMode
    {
        Never = 0,
        WhenLaunching = 1,
        WhenOpeningImages = 2
    };

    inline QSet<QString> listToSet(const QStringList &list)
    {
        return QSet<QString>{list.begin(), list.end()};
    }

    inline QStringList setToList(const QSet<QString> &set)
    {
        return QStringList(set.begin(), set.end());
    }

    inline bool scrollActionIsSelfCompatible(const ViewportScrollAction action)
    {
        return
            action == ViewportScrollAction::None ||
            action == ViewportScrollAction::Pan;
    }
}

#endif // QVNAMESPACE_H
