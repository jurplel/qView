#ifndef QVNAMESPACE_H
#define QVNAMESPACE_H

namespace Qv
{
    static constexpr int SessionStateVersion = 1;

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

    enum class ColorSpaceConversion
    {
        Disabled = 0,
        AutoDetect = 1,
        SRgb = 2,
        DisplayP3 = 3
    };

    enum class FitMode
    {
        WholeImage = 0,
        OnlyHeight = 1,
        OnlyWidth = 2
    };

    enum class PreloadMode
    {
        Disabled = 0,
        Adjacent = 1,
        Extended = 2
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

    enum class WindowResizeMode
    {
        Never = 0,
        WhenLaunching = 1,
        WhenOpeningImages = 2
    };
}

#endif // QVNAMESPACE_H
